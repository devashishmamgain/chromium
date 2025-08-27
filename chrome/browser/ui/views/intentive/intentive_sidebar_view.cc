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
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
 

#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

BEGIN_METADATA(IntentiveSidebarView)
END_METADATA

IntentiveSidebarView::IntentiveSidebarView(
  NavigationCallback navigation_callback, int width_dip)
  : navigation_callback_(std::move(navigation_callback)),
    width_dip_(width_dip) {
  InitializeView();
}

IntentiveSidebarView::IntentiveSidebarView(Browser* browser, int width_dip)
    : browser_(browser), width_dip_(width_dip) {
  InitializeView();
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

  // Container that will hold the app buttons (kept separate so the + button
  // stays while we rebuild the list).
  apps_container_ = rail_->AddChildView(std::make_unique<views::View>());
  apps_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Add button to append new app entries.
  add_button_ = rail_->AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&IntentiveSidebarView::OnAddPressed,
                          base::Unretained(this)),
      u"+"));
  add_button_->SetHorizontalAlignment(gfx::ALIGN_CENTER);

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
  std::vector<views::View*> to_remove;
  for (views::View* child : apps_container_->children())
    to_remove.push_back(child);
  for (views::View* v : to_remove)
    apps_container_->RemoveChildViewT(v);

  for (const auto& app : apps_) {
    auto* btn = apps_container_->AddChildView(std::make_unique<views::LabelButton>(
        base::BindRepeating(&IntentiveSidebarView::OnAppPressed,
                            base::Unretained(this), app),
        base::UTF8ToUTF16(app.name)));
    btn->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  }

  InvalidateLayout();
  SchedulePaint();
}

void IntentiveSidebarView::OnAppPressed(const IntentiveAppEntry& entry) {
  if (navigation_callback_)
    navigation_callback_.Run(entry.url);
}

void IntentiveSidebarView::OnAddPressed() {
  if (add_form_container_)
    return;  // Already open.
  size_t insert_index = rail_->GetIndexOf(add_button_).value_or(rail_->children().size());
  add_form_container_ = rail_->AddChildViewAt(std::make_unique<views::View>(),
                                              insert_index);
  auto* layout = add_form_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::Orientation::kVertical));
  layout->set_between_child_spacing(6);

  add_form_container_->AddChildView(std::make_unique<views::Label>(u"Add App"));
  name_field_ = add_form_container_->AddChildView(std::make_unique<views::Textfield>());
  name_field_->SetPlaceholderText(u"Name");
  url_field_ = add_form_container_->AddChildView(std::make_unique<views::Textfield>());
  url_field_->SetPlaceholderText(u"https://example.com");

  auto* actions = add_form_container_->AddChildView(std::make_unique<views::View>());
  auto* actions_layout = actions->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));
  actions_layout->set_between_child_spacing(8);
  actions->AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&IntentiveSidebarView::OnAddFormAccept,
                          base::Unretained(this)),
      u"Add"));
  actions->AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&IntentiveSidebarView::OnAddFormCancel,
                          base::Unretained(this)),
      u"Cancel"));
  InvalidateLayout();
  SchedulePaint();
}

void IntentiveSidebarView::OnAddFormAccept() {
  std::string name_utf8 = base::UTF16ToUTF8(name_field_->GetText());
  std::string url_utf8 = base::UTF16ToUTF8(url_field_->GetText());
  if (url_utf8.empty())
    return;
  if (url_utf8.find("://") == std::string::npos)
    url_utf8 = std::string("https://") + url_utf8;
  GURL url(url_utf8);
  if (!url.is_valid())
    return;
  if (name_utf8.empty())
    name_utf8 = url.host();
  apps_.push_back(IntentiveAppEntry{std::move(name_utf8), std::move(url), 0});
  // Tear down form.
  rail_->RemoveChildViewT(add_form_container_);
  add_form_container_ = nullptr;
  name_field_ = nullptr;
  url_field_ = nullptr;
  Rebuild();
}

void IntentiveSidebarView::OnAddFormCancel() {
  if (!add_form_container_)
    return;
  rail_->RemoveChildViewT(add_form_container_);
  add_form_container_ = nullptr;
  name_field_ = nullptr;
  url_field_ = nullptr;
  InvalidateLayout();
  SchedulePaint();
}
