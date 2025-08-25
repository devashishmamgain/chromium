#pragma once

#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"
#include "content/public/browser/web_ui_controller.h"

class IntentiveSidebarUI : public content::WebUIController {
 public:
  explicit IntentiveSidebarUI(content::WebUI* web_ui);
  ~IntentiveSidebarUI() override = default;
};