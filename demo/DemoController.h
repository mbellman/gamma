#include "Gamma.h"

class DemoController : public Gamma::AbstractController {
public:
  virtual void init() override;
  virtual void destroy() override;
};