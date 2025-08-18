#include "chrome/browser/ui/views/intentive/intentive_chat_overlay.h"

#include "base/memory/weak_ptr.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "url/gurl.h"

namespace {
constexpr int kW = 420;
constexpr int kH = 600;
constexpr int kPadding = 16;
}  // namespace

// static
views::Widget* IntentiveChatOverlay::ShowOrToggle(views::View* parent_view,
                                                  content::BrowserContext* context) {
  // Check if there's already an existing widget for this parent view
  views::Widget* existing_widget = nullptr;
  auto widgets = views::Widget::GetAllOwnedWidgets(
      parent_view->GetWidget()->GetNativeView());
  
  for (views::Widget* widget : widgets) {
    if (widget && widget->GetName() == "IntentiveChatOverlay") {
      existing_widget = widget;
      break;
    }
  }

  if (existing_widget) {
    // Toggle visibility
    if (existing_widget->IsVisible()) {
      existing_widget->Hide();
    } else {
      existing_widget->Show();
    }
    return existing_widget;
  }

  // No existing widget, create a new one
  auto* overlay = new IntentiveChatOverlay(context);
  auto* widget = new views::Widget();
  
  views::Widget::InitParams params(
      views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.name = "IntentiveChatOverlay";
  params.delegate = nullptr;
  params.parent = parent_view->GetWidget()->GetNativeView();
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  widget->Init(std::move(params));
  
  widget->SetContentsView(overlay);
  overlay->widget_ = widget->GetWeakPtr();

  // Position bottom-right within the parent content area
  const gfx::Rect pb = parent_view->GetBoundsInScreen();
  widget->SetBounds({pb.right() - kW - kPadding,
                     pb.bottom() - kH - kPadding,
                     kW, kH});

  widget->Show();
  return widget;
}

IntentiveChatOverlay::IntentiveChatOverlay(content::BrowserContext* context)
    : browser_context_(context) {
  SetUseDefaultFillLayout(true);
  web_view_ = AddChildView(std::make_unique<views::WebView>(browser_context_));
  web_view_->LoadInitialURL(GURL("https://chatgpt.com"));
  web_view_->SetPreferredSize(gfx::Size(kW, kH));
}

IntentiveChatOverlay::~IntentiveChatOverlay() = default;
