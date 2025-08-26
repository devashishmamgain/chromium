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
  // Remove existing children in the rail.
  std::vector<views::View*> to_remove;
  for (views::View* child : rail_->children())
    to_remove.push_back(child);
  for (views::View* v : to_remove)
    rail_->RemoveChildViewT(v);

  // Add one simple text button per app for now.
  for (const auto& app : apps_) {
    auto* btn = rail_->AddChildView(std::make_unique<views::LabelButton>(
        base::BindRepeating(&IntentiveSidebarView::OnAppPressed,
                            base::Unretained(this), app),
        base::UTF8ToUTF16(app.name)));
    btn->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  }

  InvalidateLayout();
  SchedulePaint();
}

void IntentiveSidebarView::OnAppPressed(const IntentiveAppEntry& entry) {
  if (navigation_callback_) {
    navigation_callback_.Run(entry.url);
  }
}