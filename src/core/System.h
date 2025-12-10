#ifndef DDD_CORE_SYSTEM_H
#define DDD_CORE_SYSTEM_H

class System {
  public:
    virtual ~System() = default;
    virtual void shutdown() {}
    virtual void update(float dt) = 0;
};

#endif // DDD_CORE_SYSTEM_H

