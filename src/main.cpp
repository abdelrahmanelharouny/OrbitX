#include "core/Application.h"

int main() {
  agss::Application app;
  if (!app.Init()) {
    return 1;
  }
  app.Run();
  app.Shutdown();
  return 0;
}

