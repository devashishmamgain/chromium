#ifndef CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_CHAT_OVERLAY_H_
#define CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_CHAT_OVERLAY_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace views {
class View;
class WebView;
class Widget;
class LabelButton;
}  // namespace views

namespace content {
class BrowserContext;
}  // namespace content

// Simple content view for the floating overlay. The Widget will own this view.
class IntentiveChatOverlay : public views::View {
 public:
  // Provide metadata so views::AsViewClass works without RTTI.
  METADATA_HEADER(IntentiveChatOverlay, views::View)

 public:

  // Create or toggle the floating widget above |parent_view| and return its Widget.
  static views::Widget* ShowOrToggle(views::View* parent_view,
                                   content::BrowserContext* context);

  // Ensure the overlay is visible: if it exists, show/focus it; otherwise
  // create it. Unlike ShowOrToggle, this never hides an already-visible
  // overlay.
  static views::Widget* ShowOrEnsureVisible(views::View* parent_view,
                                            content::BrowserContext* context);

  explicit IntentiveChatOverlay(content::BrowserContext* context,
                                views::View* host_parent_view);
  ~IntentiveChatOverlay() override;

  // Set the chat input text inside the embedded ChatGPT page and focus it.
  void SetPromptText(const std::u16string& text);
  // Try to click send / submit after text is set.
  void SendPrompt();
  // Public entry to scrape active page, prepare context and send to AI.
  void SendPageToAI();

 private:
  void TrySetPromptText(const std::u16string& text, int attempt);
  void TrySendPrompt(int attempt);
  void OnSendPageToAI();
  void Layout(PassKey) override;

  // Weak pointer to the widget that hosts this view.
  base::WeakPtr<views::Widget> widget_;
  raw_ptr<content::BrowserContext> browser_context_ = nullptr;
  raw_ptr<views::WebView> web_view_ = nullptr;
  // No header/button inside overlay anymore; control is in toolbar.
  // The container view from which we can find the active page WebContents.
  raw_ptr<views::View> host_parent_view_ = nullptr;

  base::WeakPtrFactory<IntentiveChatOverlay> weak_ptr_factory_{this};
};

#endif  // CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_CHAT_OVERLAY_H_
