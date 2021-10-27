#include "Gamma.h"

class DemoScene : public Gamma::AbstractScene {
public:
  virtual void init() override;
  virtual void destroy() override;
  virtual void update(float dt) override;

private:
  int lastRemovedIndex = 0;
  Gamma::Light* clight = nullptr;

  void addFloor();
  void addCubesExhibit();
};