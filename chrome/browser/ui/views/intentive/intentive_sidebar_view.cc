#include "chrome/browser/ui/views/intentive/intentive_sidebar_view.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/common/intentive/intentive_prefs.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/border.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/base/ui_base_types.h"
#include "ui/base/models/image_model.h"
#include "components/vector_icons/vector_icons.h"

// Favicon fetching
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/favicon/favicon_utils.h"
#include "components/favicon/core/favicon_service.h"
#include "components/keyed_service/core/service_access_type.h"

namespace {
class AddAppDialog : public views::DialogDelegate {
 public:
  using AcceptCallback = base::OnceCallback<void(std::string, GURL)>;

  explicit AddAppDialog(AcceptCallback cb) : on_accept_(std::move(cb)) {}

  std::u16string GetWindowTitle() const override { return u"Add App"; }
  ui::mojom::ModalType GetModalType() const override {
    return ui::mojom::ModalType::kWindow;
  }

  bool Accept() override {
    std::string name_utf8 = base::UTF16ToUTF8(name_field_->GetText());
    std::string url_utf8 = base::UTF16ToUTF8(url_field_->GetText());
    if (url_utf8.empty())
      return false;
    if (url_utf8.find("://") == std::string::npos)
      url_utf8 = std::string("https://") + url_utf8;
    GURL url(url_utf8);
    if (!url.is_valid())
      return false;
    if (name_utf8.empty())
      name_utf8 = url.host();
    std::move(on_accept_).Run(name_utf8, url);
    return true;
  }

  views::View* GetContentsView() override {
    if (!contents_) {
      auto contents = std::make_unique<views::View>();
      auto* layout = contents->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
      layout->set_inside_border_insets(gfx::Insets::TLBR(16, 16, 16, 16));
      layout->set_between_child_spacing(10);

      contents->AddChildView(std::make_unique<views::Label>(u"Name"));
      name_field_ = contents->AddChildView(std::make_unique<views::Textfield>());
      name_field_->SetPlaceholderText(u"App name");

      contents->AddChildView(std::make_unique<views::Label>(u"URL"));
      url_field_ = contents->AddChildView(std::make_unique<views::Textfield>());
      url_field_->SetPlaceholderText(u"https://example.com");

      contents_ = contents.get();
      owned_contents_ = std::move(contents);
    }
    return contents_;
  }

 private:
  raw_ptr<views::View> contents_ = nullptr;
  std::unique_ptr<views::View> owned_contents_;
  raw_ptr<views::Textfield> name_field_ = nullptr;
  raw_ptr<views::Textfield> url_field_ = nullptr;
  AcceptCallback on_accept_;
};
}  // namespace
// keep includes above grouped logically


#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

BEGIN_METADATA(IntentiveSidebarView)
END_METADATA

IntentiveSidebarView::IntentiveSidebarView(
  NavigationCallback navigation_callback, int width_dip)
  : navigation_callback_(std::move(navigation_callback)),
    width_dip_(width_dip) {
  favicon_task_tracker_ = std::make_unique<base::CancelableTaskTracker>();
  InitializeView();
}

IntentiveSidebarView::IntentiveSidebarView(Browser* browser, int width_dip)
    : browser_(browser), width_dip_(width_dip) {
  favicon_task_tracker_ = std::make_unique<base::CancelableTaskTracker>();
  InitializeView();
}

void IntentiveSidebarView::SetBrowser(Browser* browser) {
  browser_ = browser;
  // Load apps from prefs if available and non-empty.
  if (browser_ && browser_->profile()) {
    PrefService* prefs = browser_->profile()->GetPrefs();
    const base::Value::List& list = prefs->GetList(intentive::kIntentiveSidebarApps);
    if (!list.empty()) {
      std::vector<IntentiveAppEntry> loaded;
      for (const auto& v : list) {
        const base::Value::Dict& d = v.GetDict();
        const std::string* name = d.FindString("name");
        const std::string* url = d.FindString("url");
        if (!url)
          continue;
        IntentiveAppEntry e;
        e.name = name ? *name : GURL(*url).host();
        e.url = GURL(*url);
        if (e.url.is_valid())
          loaded.push_back(std::move(e));
      }
      if (!loaded.empty()) {
        apps_ = std::move(loaded);
        Rebuild();
      }
    }
  }
}

void IntentiveSidebarView::InitializeView() {
  SetUseDefaultFillLayout(false);
  auto* root_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));
  root_layout->set_inside_border_insets(gfx::Insets::TLBR(0, 0, 0, 0));
  root_layout->set_between_child_spacing(0);

  rail_ = AddChildView(std::make_unique<views::View>());
  auto* rail_layout = rail_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  rail_layout->set_inside_border_insets(gfx::Insets::TLBR(8, 8, 8, 8));
  rail_layout->set_between_child_spacing(8);

  // Scrollable container for app buttons.
  apps_scroll_ = rail_->AddChildView(std::make_unique<views::ScrollView>());
  apps_scroll_->SetDrawOverflowIndicator(false);
  apps_scroll_->SetHorizontalScrollBarMode(views::ScrollView::ScrollBarMode::kDisabled);
  apps_container_ = apps_scroll_->SetContents(std::make_unique<views::View>());
  apps_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Add button to append new app entries.
  auto* add_btn = rail_->AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&IntentiveSidebarView::OnAddPressed,
                          base::Unretained(this))));
  add_btn->SetImageModel(views::Button::STATE_NORMAL,
                         ui::ImageModel::FromVectorIcon(vector_icons::kAddIcon));
  add_btn->SetTooltipText(u"Add app");
  add_btn->SetPreferredSize(gfx::Size(28, 28));
  add_button_ = nullptr; // not used for ImageButton, keep member for layout index

  content_container_ = AddChildView(std::make_unique<views::View>());
}

IntentiveSidebarView::~IntentiveSidebarView() = default;

void IntentiveSidebarView::SetApps(std::vector<IntentiveAppEntry> apps) {
  apps_ = std::move(apps);
  Rebuild();
}

gfx::Size IntentiveSidebarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  constexpr int kDefaultHeight = 800;
  const int h = available_size.height().is_bounded()
                    ? available_size.height().value()
                    : kDefaultHeight;
  return gfx::Size(width_dip_, h);
}

void IntentiveSidebarView::Rebuild() {
  // Clear and rebuild just the apps container, preserving the + button.
  if (!apps_container_)
    return;
  // Cancel any in-flight favicon requests before destroying buttons.
  if (favicon_task_tracker_)
    favicon_task_tracker_->TryCancelAll();
  std::vector<views::View*> to_remove;
  for (views::View* child : apps_container_->children())
    to_remove.push_back(child);
  for (views::View* v : to_remove)
    apps_container_->RemoveChildViewT(v);

  for (size_t i = 0; i < apps_.size(); ++i) {
    const auto& app = apps_[i];
    auto* btn = apps_container_->AddChildView(std::make_unique<views::LabelButton>(
        base::BindRepeating(&IntentiveSidebarView::OnAppPressed,
                            base::Unretained(this), app),
        base::UTF8ToUTF16(app.name)));
    btn->SetTooltipText(base::UTF8ToUTF16(app.url.spec()));
    btn->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    btn->SetBorder(views::CreateEmptyBorder(gfx::Insets::TLBR(8, 10, 8, 10)));
    // Set a default favicon first, then fetch real one asynchronously.
    btn->SetImageLabelSpacing(8);
    btn->SetImageModel(views::Button::STATE_NORMAL,
                       favicon::GetDefaultFaviconModel());
    LoadButtonIconForUrl(btn, app.url);
    const bool selected = static_cast<int>(i) == selected_index_;
    const SkColor bg = selected ? SkColorSetARGB(40, 255, 255, 255)
                                : SK_ColorTRANSPARENT;
    btn->SetBackground(views::CreateSolidBackground(bg));
  }

  InvalidateLayout();
  SchedulePaint();
}

void IntentiveSidebarView::OnAppPressed(const IntentiveAppEntry& entry) {
  for (size_t i = 0; i < apps_.size(); ++i) {
    if (apps_[i].url == entry.url) {
      selected_index_ = static_cast<int>(i);
      break;
    }
  }
  Rebuild();
  if (navigation_callback_)
    navigation_callback_.Run(entry.url);
}

void IntentiveSidebarView::OnAddPressed() {
  auto* parent_widget = GetWidget();
  auto* dialog = new AddAppDialog(base::BindOnce(
      &IntentiveSidebarView::OnAppAdded, base::Unretained(this)));
  views::Widget* widget = views::DialogDelegate::CreateDialogWidget(
      dialog, parent_widget ? parent_widget->GetNativeWindow() : gfx::NativeWindow(),
      parent_widget ? parent_widget->GetNativeView() : gfx::NativeView());
  widget->Show();
}

void IntentiveSidebarView::OnAppAdded(std::string name, GURL url) {
  apps_.push_back(IntentiveAppEntry{std::move(name), std::move(url), 0});
  // Persist to prefs if available.
  if (browser_ && browser_->profile()) {
    base::Value::List out;
    for (const auto& a : apps_) {
      base::Value::Dict d;
      d.Set("name", a.name);
      d.Set("url", a.url.spec());
      out.Append(std::move(d));
    }
    browser_->profile()->GetPrefs()->SetList(intentive::kIntentiveSidebarApps,
                                             std::move(out));
  }
  Rebuild();
}

void IntentiveSidebarView::LoadButtonIconForUrl(views::LabelButton* button,
                                                const GURL& url) {
  if (!button)
    return;
  if (!browser_ || !browser_->profile())
    return;

  favicon::FaviconService* favicon_service =
      FaviconServiceFactory::GetForProfile(browser_->profile(),
                                           ServiceAccessType::EXPLICIT_ACCESS);
  if (!favicon_service)
    return;

  favicon_service->GetFaviconImageForPageURL(
      url,
      base::BindOnce(
          [](views::LabelButton* target,
             const favicon_base::FaviconImageResult& result) {
            if (!target)
              return;
            if (!result.image.IsEmpty()) {
              target->SetImageModel(views::Button::STATE_NORMAL,
                                    ui::ImageModel::FromImage(result.image));
            }
          },
          base::Unretained(button)),
      favicon_task_tracker_.get());
}
