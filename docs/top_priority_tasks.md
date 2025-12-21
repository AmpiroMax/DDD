# Top priority tasks — Diablo-like Isometric PvP (8 players)

Этот файл фиксирует **задачи высшего приоритета** для ветки `diablo`:
- **Изометрический рендер (2D)**
- **Топ-даун физика/коллизии**
- **PvP арена (deathmatch) до 8 игроков**
- **Сетевой дизайн** (и затем реализация)
- После стабилизации: **заклинания/призывы/баланс**

Старый план (scheduler/EventBus/reset и т.п.) держим как бэклог и трогаем только если он блокирует новые эпики.

---

## Owners (кто за что отвечает)

### EPIC A — Isometric renderer
- **Главный**: график
- **Партнёры**: механик UI (debug/оверлеи), механик (интеграция режимов/камера), физик (визуализация коллизий)

### EPIC B — Top-down physics & movement
- **Главный**: физик
- **Партнёры**: механик (параметры “feel” и интеграция), график (debug drawing/спрайтовые якоря), механик UI (переключатели debug)

### EPIC C — PvP Arena slice (deathmatch)
- **Главный**: механик
- **Партнёры**: график (арена-храм ассеты), физик (коллайдеры арены), механик UI (табло/таймер/респавн)

### EPIC D — Networking (design → prototype)
- **Главный**: механик
- **Партнёры**: физик (детерминизм/симуляция), механик UI (net debug), механик инвентаря (модель данных под спеллы/слоты)

### EPIC E — Spells / Summons / Balance
- **Главный**: механик инвентаря (как владелец data-driven/слотов/конфигов)
- **Партнёры**: механик (правила PvP/баланс), график (VFX/иконки), физик (коллизии снарядов/AoE), механик UI (кастбар/кулдауны/хотбар)

## EPIC A — Isometric renderer (играбельная сцена)

### Цель
Стабильный изометрический рендер окружения и персонажей: камера, масштаб, сортировка по глубине, простое освещение/слои.

### Definition of Done (минимум)
- Изометрическая камера (пан/зум) + корректная проекция world→screen.
- Персонажи/объекты сортируются по depth (минимум: по `y`, с tie-breaker по слою).
- Есть “арена-храм” (хотя бы прототип) и она корректно рисуется (пол, колонны, стены).
- Debug overlay: показать координаты, FPS, хитбоксы (по флагу).

### Вехи
- **A0 — Iso camera MVP**: пан/зум, конвертеры координат, debug grid.
- **A1 — Depth sorting**: порядок отрисовки (y/layer), валидация на колоннах/стенах.
- **A2 — Environment pass**: тайлы/декор/колонны как объекты, батчинг где возможно.
- **A3 — Character pass**: анимации (idle/run), тени/контур (минимум).
- **A4 — Visual debug**: хитбоксы, навмеш/коллизии, сетевые “ghosts” (позже).

### Документ
См. `docs/isometric_render.md`.

---

## EPIC B — Top-down physics & movement (feel)

### Цель
Сделать управление персонажем в изометрии: движение, ускорение/трение, коллизии со стенами/колоннами, корректная работа 2–8 игроков (пока локально).

### Definition of Done (минимум)
- Движение в плоскости (x,y) с “feel” (accel/decel, диагонали).
- Коллизии персонажа со статикой (стены/колонны) без застреваний.
- Настраиваемые параметры (speed/accel/radius) через конфиг.
- Debug: визуализация коллайдеров + столкновений.

### Вехи
- **B0 — Input→move MVP**: WASD, нормализация, dt.
- **B1 — Collision MVP**: circle-vs-AABB / circle-vs-segment (выбрать один и закрепить).
- **B2 — Map colliders**: коллайдеры арены (стены/колонны), загрузка из данных карты.
- **B3 — Multi-entity**: 8 игроков локально (боты/куклы), разделение коллизий player-player (пуш/без пуша).

### Документ
См. `docs/pvp_arena.md` (геометрия) и `docs/isometric_render.md` (визуал).

---

## EPIC C — PvP Arena slice (deathmatch)

### Цель
Играбельная арена “храм с колоннами” с базовыми правилами deathmatch (очки, респавны, таймер), рассчитанная на 2–8 игроков.

### Definition of Done (минимум)
- 8 игроков могут одновременно бегать по арене (локально).
- Есть спавн-точки, респавн, счёт/таймер.
- Есть PvP зона (границы/правила) — хотя бы 1 карта.

---

## EPIC D — Networking (design → prototype)

### Цель
Зафиксировать сетевую архитектуру для PvP (до 8 игроков): authoritative server, репликация, античит базового уровня, лаг-компенсация на выстрелы/касты (минимум).

### Definition of Done (design)
- Документ с решениями: модель (client-server), протокол/сообщения, tickrate, репликация, prediction/reconciliation (минимум), тестовый план.

---

## EPIC E — Spells / Summons / Balance (после стабилизации сцены)

### Цель
Пайплайн заклинаний (projectiles, AoE, summon entities) + базовый баланс под deathmatch.

### Definition of Done (MVP)
- 3–5 заклинаний, включая 1 призыв (summon), работают в PvP.
- Есть базовые статы (hp/mana, cooldowns, damage types) и конфиги.

---

## APPENDIX A — Architecture backlog (previous plan)

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

---

## TASK 5 — ResetPipeline: детерминированный reset как транзакция (Quiesce → Unhook → Destroy → Recreate → Resume)

### Название
**World reset as transaction: ResetPipeline + ResetPhases (no “ad-hoc resetWorld”)**

### Проблема
Reset мира нужен для SwitchMap/Restart/LoadSave/ExitToMenu, но сейчас это делается через набор разрозненных вызовов (`PhysicsManager.resetWorld` с placement-new, `EntityManager.clear`, ручные `physicsSystem->reset`, перецепление listener в update и т.п.).

Это опасно, потому что:
- после reset любые старые `b2Body*` становятся невалидны (UAF риск),
- системы могут продолжить реагировать на события во время reset (если нет токенов/отписок),
- сложно гарантировать порядок действий и повторяемость.

### Что обсуждали / от чего отказались
- **Отказались от “на скорую руку”**: не хотим держать reset как набор вызовов в GameApp, который легко сломать перестановкой строк.
- **Отказались от динамического DAG/reset-магии**: reset должен быть прозрачным и детерминированным.

### Что решили делать (и почему)
Сделать reset **транзакцией** с явными фазами:
1) **Quiesce**: остановить gameplay/physics/render (scheduler переводится в ResetMode/ResetPhase).
2) **Unhook**: системы отцепляют внешние ресурсы/колбэки (например, PhysicsSystem снимает contact listener, чистит кэши коллайдеров).
3) **Destroy**: очистка ECS-энтити и физики.
4) **Recreate**: создание tilemap/player/drops и применение сейва/карты.
5) **Resume**: вернуть рабочие фазы и включённые системы.

Почему так:
- невозможно “случайно” оставить висящие указатели,
- порядок фиксирован и документирован,
- подготовка к scheduler (SystemsSystem) и EventBus tokens.

### Затрагивает
- `src/game/GameApp.cpp/.h` (перенос reset/start/load/save к единообразному протоколу)
- `src/systems/PhysicsSystem.h` (явные хуки reset/init)
- новые файлы (предпочтительно):
  - `src/systems/ResetSystem.h/.cpp` (как System-исполнитель reset протокола)
  - `src/managers/ResetManager.h` (как интерфейс для меню/загрузки сейва)

### Шаги реализации
1) Ввести понятие `ResetReason` (SwitchMap/Restart/LoadSave/ExitToMenu) + `ResetContext` (mapPath, savePath).
2) Реализовать ResetPhase/ResetMode в scheduler (или временно в GameApp): во время reset не тикаем gameplay/physics/render.
3) Реализовать фазу Unhook:
   - `PhysicsSystem.shutdown()` (снять listener) и `PhysicsSystem.reset()` (очистить tileBodies/кэши).
   - (опционально) UI/другие системы отцепляют внешние pointers.
4) Destroy:
   - `EntityManager.clear()`
   - физику очищаем через TASK 6 (clearWorld без пересоздания b2World).
5) Recreate:
   - `loadMapAndEntities(mapPath)` или `loadSave(savePath)` через единый поток, без дублей логики.
   - убедиться, что `TilemapComponent.originalTiles` заполнен корректно.
6) Resume:
   - включить нужные системы/фазы (Playing/Menu) и вернуться в цикл.

### Исполнители
- **Senior/TL (Core)**: протокол фаз, интеграция с scheduler и меню.
- **Physics developer**: хуки reset/init в PhysicsSystem, проверка tileBodies/contact listener.
- **UI developer**: переходы экранов, инициирование reset (SwitchMap/ExitToMenu/Continue).

---

## TASK 6 — PhysicsManager: clearWorld вместо placement-new resetWorld + правила lifetime Box2D

### Название
**Safe physics reset: clearWorld() (destroy all bodies) + lifetime invariants (no placement-new)** 

### Проблема
`PhysicsManager.resetWorld()` сейчас пересоздаёт `b2World` через placement-new. Это усиливает риск UAF: любые указатели/ссылки на мир/тела становятся невалидны, а порядок reset обязан быть идеальным.

### Что обсуждали / от чего отказались
- **Отказались от “пересоздаём b2World как объект”** для основательного решения: слишком легко словить тонкие баги, особенно при появлении scheduler и enable/disable систем.

### Что решили делать (и почему)
Сделать `PhysicsManager.clearWorld()`:
- не меняем объект `b2World`,
- проходим по списку тел и `DestroyBody` для всех,
- (опционально) сбрасываем listener через PhysicsSystem shutdown.

Почему так:
- адрес `b2World` стабилен,
- проще reasoning про lifetime,
- меньше неожиданных side-effects.

### Затрагивает
- `src/managers/PhysicsManager.h`
- `src/game/GameApp.cpp` (вызовы reset физики)
- `src/systems/PhysicsSystem.h` (поведение при reset: ожидать empty world)

### Шаги реализации
1) Добавить `PhysicsManager::clearWorld()` (итерация по `GetBodyList()` с сохранением next).
2) Постепенно заменить использование `resetWorld()` на `clearWorld()` в reset pipeline (TASK 5).
3) Удалить/задепрекейтить `resetWorld()` или оставить только для тестов, но запретить в прод-коде.
4) Зафиксировать инвариант:
   - reset всегда делает: shutdown physics → clear bodies → clear entities → recreate → reattach listener.

### Исполнители
- **Physics developer**: реализация clearWorld + согласование с PhysicsSystem reset.
- **Senior/TL (Core)**: интеграция в ResetPipeline и контроль инвариантов.

---

## TASK 7 — Decompose `GameApp.cpp`: убрать бизнес-логику из entrypoint (один исполнитель)

### Название
**GameApp.cpp shrink to composition-root: вынести Menu/Flow, Save/Load, WorldBuild, Config/Resources в Systems+Managers**

### Проблема
`src/game/GameApp.cpp` (~1000 строк) сейчас является “god-file” и содержит сразу несколько доменов:
- main loop + порядок апдейта подсистем,
- UI flow (MainMenu/MapSelect/Playing/Pause/Settings) + обработка кликов,
- load config/game.json,
- загрузка ресурсов и ручная регистрация atlas регионов,
- world building (создание Tilemap/Player/Drops),
- save/load (JSON, diff tiles, восстановление сущностей),
- reset/переходы экранов.

Это плохая точка концентрации логики: entrypoint должен быть “сухим” и делать только высокоуровневые вызовы.
Риски:
- скрытые зависимости и повторение «двух правд» (как было с Transform vs PhysicsBody),
- трудно тестировать/дебажить (любой баг превращается в расследование по GameApp),
- любые новые фичи раздувают entrypoint (и ломают соседние подсистемы).

### Что обсуждали / от чего отказались (и почему)
- **Не делаем “косметику”**: не просто выносим static helper-функции внутри GameApp — это прячет проблему, но не фиксит ownership и зависимости.
- **Не делаем мегасистему “всё-в-одном”**: иначе получим новый god-file под другим именем.
- **Не вводим динамический DAG/job-graph** на этом шаге: сначала фиксируем ownership и детерминизм (scheduler/reset/eventbus), потом можно усложнять.

### Что решили делать (и почему)
Свести `GameApp` к роли **composition-root**:
- создаёт менеджеры/системы,
- регистрирует их в scheduler (TASK 1–2),
- вызывает высокоуровневые методы (flow/reset/save/load).

Бизнес-логика и “склейка” выносятся в отдельные доменные модули в вашей парадигме **System + Manager**.
Цель: `GameApp.cpp` ~150–250 строк, без крупных helper-реализаций.

### Затрагивает
- `src/game/GameApp.cpp`, `src/game/GameApp.h` (радикально упрощаются)
- новые файлы (предпочтительно):
  - `src/systems/GameFlowSystem.h/.cpp` + `src/managers/GameFlowManager.h` (экраны/переходы/меню)
  - `src/systems/SaveSystem.h/.cpp` + `src/managers/SaveManager.h` (сейв/лоад)
  - `src/systems/WorldBuilderSystem.h/.cpp` + `src/managers/WorldManager.h` (создание мира/сущностей)
  - `src/systems/ConfigSystem.h/.cpp` + `src/managers/ConfigManager.h` (чтение config/*.json)
  - `src/systems/ResourceBootstrapSystem.h/.cpp` + `src/managers/ResourceBootstrapManager.h` (инициализация ресурсов/атласа)

### Подзадачи (3–7 подпунктов, выполняет один человек, строго по порядку)
1) **Cut composition-root** (цель и границы GameApp)
   - Зафиксировать “что остаётся”: создание managers, initSystems, run-loop, high-level вызовы.
   - Запретить в GameApp: определение структур SaveData/MenuState/arena data и др. (вынести).

2) **ConfigSystem/ConfigManager**
   - Перенести разбор `config/game.json` (+ связанный runtime state) в отдельный модуль.
   - GameApp получает готовый `GameConfig` через manager API (без JSON ключей в GameApp).

3) **ResourceBootstrapSystem/Manager**
   - Перенести `loadResources()` (font candidates, tileset selection, atlas region registration) из GameApp.
   - Уточнить источник правды для atlas regions (конфиг/дата-файл), чтобы не держать “таблицы регионов” в GameApp.

4) **WorldBuilderSystem/WorldManager**
   - Перенести `loadMapAndEntities()` в модуль мира:
     - создание tilemap entity + player entity + базовых props,
     - единые фабрики сущностей (Player, Drop, ArenaStatic).
   - Важно: модуль мира должен знать, что является authoritative (Tilemap.tiles и т.п.), а GameApp нет.

5) **SaveSystem/SaveManager**
   - Перенести `collectSaveData()/applySaveData()/loadSave()/saveGame()` из GameApp.
   - Зафиксировать формат сейва и инварианты (what is authoritative) рядом с SaveSystem, а не в entrypoint.

6) **GameFlowSystem/GameFlowManager**
   - Перенести UI flow/меню: buildMenuButtons/updateMenu/handleMenuClicks, экраны и переходы.
   - GameFlow инициирует ResetPipeline (TASK 5) и вызывает SaveSystem.

7) **Final cleanup** (удалить “хвосты” из GameApp)
   - Удалить оставшиеся helper-функции и структуры, оставив только композицию и вызовы менеджеров.
   - Обновить docs (`docs/systems_overview.md`/`docs/overview_and_map.md` при необходимости).

### Исполнители
Один человек (желательно **Senior/TL (Core)**), потому что затрагивает композицию, зависимость всех подсистем и стиль проекта.
Партнёрские ревью точечно:
- UI developer: проверить, что меню/flow не сломан и визуально корректен.
- Physics developer: проверить, что reset/world build не ломает физику и коллайдеры.
