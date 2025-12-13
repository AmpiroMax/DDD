# High priority tasks (architecture)

Этот файл фиксирует **задачи высшего приоритета** по результатам аудита и обсуждения.
Фокус: убрать «две правды», сделать порядок апдейта детерминированным, и безопасно управлять подписками EventBus.

---

## TASK 1 — SystemsSystem (scheduler) + SystemsManager (управление)

### Название
**Deterministic Systems Scheduler (variant A: vector + priority + phases) + management API**

### Проблема
Сейчас порядок и условия апдейта систем размазаны по `GameApp` (несколько списков `updateSystems`, `renderSystems`, отдельный `physicsSystem`, ручные вызовы `eventBus.pump()`, `updateMenu()` и т.п.). Это уже приводило к «двум источникам правды» (и будет приводить снова), потому что:
- разные подсистемы обновляются разными путями,
- сложно гарантировать стабильный порядок,
- сложно безопасно включать/выключать системы.

### Что обсуждали / от чего отказались
- **Отказались от priority_queue**: она неудобна для «вызови все системы каждый кадр в стабильном порядке», а изменения (enable/disable/phase change) приводят к пересборке/перекладыванию и потере прозрачности.
- **Отказались от DAG/job-graph динамического планировщика** до релиза: сложно обеспечить детерминизм и не завезти новый класс багов.

### Что решили делать (и почему)
**Variant A**: `std::vector<Entry>` + сортировка один раз + дальнейшие изменения через флаги.
- Это даёт **детерминизм** и **быструю смену активности (O(1))**.
- Обновление списка выполняется через `enabled`/`phaseMask` без вставок/удалений.

Архитектура по вашей парадигме «System + Manager»:
- `SystemsSystem` — это `System`, хранит entries и выполняет апдейт.
- `SystemsManager` — manager-интерфейс, которым пользуются остальные (enable/disable/setPhase).

### Затрагивает
- `src/core/System.h` (возможное расширение интерфейса жизненного цикла)
- новые файлы:
  - `src/systems/SystemsSystem.h/.cpp`
  - `src/managers/SystemsManager.h` (или `src/managers/SystemsManager.h/.cpp` если понадобится реализация)

### Предлагаемые сущности
- `enum class Phase { Variable, Fixed, Render };`
- `using PhaseMask = uint32_t;`
- `enum class SystemId { Input, Inventory, PlayerControl, CameraFollow, TileInteraction, DropPickup, Physics, Render, UI, Debug, ... };`
- `struct Entry { SystemId id; System* sys; int priority; PhaseMask phases; bool enabled; uint32_t order; };`

### Шаги реализации
1) Добавить `SystemId`, `Phase`, `PhaseMask` (внутри `SystemsSystem` или в отдельном core-хедере).
2) Реализовать `SystemsSystem`:
   - `registerSystem(id, System&, priority, phases, enabled=true)`
   - хранить `entries` + `nextOrder`.
   - `finalize()` или автоматическая сортировка при регистрации (лучше finalize после регистрации всех).
   - `updatePhase(Phase, float dt)` — проход по `entries` с фильтром `enabled && (phases & currentPhase)`.
3) Реализовать **deferred операции**:
   - `requestEnable(id,bool)` / `requestPhaseMask(id,mask)` копятся в `pendingOps`.
   - `applyPending()` выполняется **после** апдейта всех систем текущего кадра.
4) Реализовать `SystemsManager` как фасад над `SystemsSystem`:
   - `enable/disable`, `setPhase` (если нужно), `applyPending`.

### Исполнители
- **Senior/TL (Core)**: проектирование API, реализация `SystemsSystem/SystemsManager`, выбор приоритетов.

---

## TASK 2 — Перенос цикла обновления GameApp на SystemsSystem (включая Render/UI)

### Название
**Move update loop orchestration from GameApp to SystemsSystem phases (Variable/Fixed/Render)**

### Проблема
Сейчас `GameApp` является оркестратором с несколькими разными списками и особыми правилами (например, input обновлялся отдельным путём, physics fixed-step отдельно, render/UI отдельно). Это снижает прозрачность и ведёт к рассинхронам.

### Что обсуждали / от чего отказались
- **Не оставляем Render/UI отдельно**: по решению — Render/UI должны быть частью общего расписания.
- **Не делаем динамический «подложи систему и перестрой граф»**: только deferred enable/disable/phase changes.

### Что решили делать (и почему)
Сделать `SystemsSystem` единственным местом, где определён порядок апдейта:
- Variable systems: input + gameplay + debug
- Fixed systems: physics
- Render systems: render + ui

`GameApp` оставляет только:
- сбор dt,
- накопление accumulator,
- вызов `SystemsSystem.updatePhase(...)` + `eventBus.pump()`.

### Затрагивает
- `src/game/GameApp.cpp`, `src/game/GameApp.h`

### Шаги реализации
1) Добавить в `GameApp` поля:
   - `SystemsSystem systemsSystem;`
   - `SystemsManager systemsManager;` (или ссылки/владение)
2) В `initSystems()`:
   - создаём реальные System объекты как сейчас, но **регистрируем** их в `systemsSystem.registerSystem(...)` с приоритетами и фазами.
   - убрать `updateSystems`/`renderSystems` вектора (или оставить временно, но не использовать).
3) В `run()` заменить:
   - variable апдейт → `systemsSystem.updatePhase(Phase::Variable, dt)`
   - fixed-step → в while-loop `systemsSystem.updatePhase(Phase::Fixed, PHYSICS_TIMESTEP)`
   - `eventBus.pump()` (между fixed и render, как сейчас)
   - render → `systemsSystem.updatePhase(Phase::Render, dt)`
4) Фазы меню/паузы:
   - вместо «if menuActive skip» — переключать `enabled`/`phaseMask` в `SystemsManager` (deferred) при входе/выходе из меню.

### Исполнители
- **Senior/TL (Core)**: перенос run-loop, назначение приоритетов, согласование фаз.
- **UI developer**: убедиться, что UI корректно работает в Phase::Render.
- **Physics developer**: убедиться, что fixed-step физики вызывается нужное число раз и синк трансформов не ломается.

---

## TASK 3 — EventBus: токены подписки + безопасная отписка

### Название
**EventBus unsubscribe tokens (RAII) + system-safe subscriptions**

### Проблема
`EventBus` сейчас не умеет unsubscribe. Подписки захватывают `this` в лямбдах. При пересоздании/отключении систем (что станет реальностью со scheduler) это может привести к:
- use-after-free,
- “призрачным” обработчикам, которые продолжают работать при выключенной системе.

### Что обсуждали / от чего отказались
- Отказались от «оставим как есть и будем надеяться, что системы живут вечно» — потому что мы вводим управление включением/выключением.

### Что решили делать (и почему)
- `EventBus::subscribe<E>(handler)` возвращает **SubscriptionToken**.
- Token хранит ссылку/указатель на bus + идентификатор подписки.
- Token отписывается в деструкторе (RAII).
- Системы хранят токены (вектор или поля) и чистят в `shutdown()`.

### Затрагивает
- `src/core/EventBus.h`
- все системы, которые подписываются:
  - `src/systems/PhysicsSystem.h`
  - `src/systems/InventorySystem.cpp`
  - `src/systems/UIRenderSystem.cpp`
  - `src/systems/PlayerControlSystem.cpp`
  - (и любые будущие)

### Шаги реализации
1) В `EventBus`:
   - добавить внутренний `HandlerId` (например `uint64_t`) + таблицу `typeid -> map<HandlerId, fn>`.
   - `subscribe<E>` возвращает `SubscriptionToken` с `{busPtr, type_index, handlerId}`.
   - `unsubscribe(type_index, handlerId)` удаляет handler.
2) В системах:
   - заменить `eventBus.subscribe<...>(lambda)` на сохранение токена(ов) в поле системы.
   - в `shutdown()` токены уничтожаются/очищаются.
3) Убедиться, что `SystemsSystem` при disable/enable может вызывать `shutdown()`/`initSubscriptions()` (если решите включать/выключать подписки динамически).

### Исполнители
- **Senior/TL (Core)**: реализация EventBus tokens, миграция всех subscribe в проекте.

---

## TASK 4 — Политика enable/disable систем и событий (без DAG)

### Название
**Deterministic enable/disable policy: deferred ops + optional one-shot tasks (no dynamic DAG)**

### Проблема
Нужно уметь менять набор активных систем (меню/пауза/экран настроек), не ломая порядок и не создавая “полкадра одно, полкадра другое”. Также важно, чтобы выключенные системы не продолжали реагировать на события.

### Что обсуждали / от чего отказались
- Отказались от динамического графа зависимостей/«подложи следующую систему и пересобери порядок» до релиза: риск недетерминизма и новых багов.

### Что решили делать (и почему)
- Все изменения активности — **deferred** (применяются после завершения текущего кадра).
- Динамика допускается только в виде:
  - deferred enable/disable/phase change,
  - (опционально) one-shot entry, который выполняется один раз и удаляется.
- События:
  - либо subscribe/unsubscribe через токены при enable/disable,
  - либо guard в обработчиках по `enabled`.
  - (предпочтение после TASK 3): управлять через tokens.

### Затрагивает
- `src/systems/SystemsSystem.*`, `src/managers/SystemsManager.*`
- системы с событиями (если подписки будут включаемыми/выключаемыми)

### Шаги реализации
1) В `SystemsSystem`: добавить очередь `pendingOps` (enable/disable/changePhase).
2) В `GameApp`: при переходах экрана (MainMenu/Playing/Pause/Settings) вызывать `systemsManager.requestEnable/Disable(...)` для нужных систем.
3) Синхронизировать с EventBus tokens:
   - выключение системы должно приводить к `shutdown()` (отписки) или явной отписке.
4) (Опционально) добавить `requestOneShot(priority, fn)` для редких задач, но не превращать это в граф зависимостей.

### Исполнители
- **Senior/TL (Core)**: политика deferred ops и интеграция с переходами экранов.
- **UI developer**: определить, какие UI/menu системы активны в каких фазах.

---

## Примечание по «менеджеры vs системы» (фиксируем модель)
Текущий проект исторически ближе к паттерну:
- **Managers = сервисы/ресурсы** (Window/Camera/Resource/Physics/Time/Debug)
- **Systems = поведение**
- **GameApp = оркестратор**

Мы эволюционируем к:
- **SystemsSystem/SystemsManager = явный оркестратор**, вместо логики в GameApp.

Это не требует немедленно делать manager 1:1 для каждой системы. Можно мигрировать поэтапно после релиза.
