#include "chrome/browser/ui/views/intentive/intentive_chat_overlay.h"

#include "base/memory/weak_ptr.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/background.h"
#include "ui/base/ui_base_types.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view_utils.h"
#include "url/gurl.h"
#include "base/strings/utf_string_conversions.h"
#include "base/json/string_escape.h"
#include "base/functional/bind.h"
#include <functional>
#include "base/functional/callback_helpers.h"
#include <memory>
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/isolated_world_ids.h"
#include "base/strings/strcat.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"

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
  auto* overlay = new IntentiveChatOverlay(context, parent_view);
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

// static
views::Widget* IntentiveChatOverlay::ShowOrEnsureVisible(
    views::View* parent_view,
    content::BrowserContext* context) {
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
    if (!existing_widget->IsVisible()) {
      existing_widget->Show();
    } else {
      // Make sure it's on top and focused.
      existing_widget->Activate();
    }
    return existing_widget;
  }

  // Fall back to creating a new one.
  return IntentiveChatOverlay::ShowOrToggle(parent_view, context);
}

IntentiveChatOverlay::IntentiveChatOverlay(content::BrowserContext* context,
                                           views::View* host_parent_view)
    : browser_context_(context), host_parent_view_(host_parent_view) {
  SetUseDefaultFillLayout(false);
  // Add the content WebView first. Header is a separate child widget layered
  // above to ensure clickability over native content.
  web_view_ = AddChildView(std::make_unique<views::WebView>(browser_context_));
  web_view_->LoadInitialURL(GURL("https://chatgpt.com"));
  web_view_->SetPreferredSize(gfx::Size(kW, kH));

  auto send_cb = base::BindRepeating(&IntentiveChatOverlay::OnSendPageToAI,
                                     base::Unretained(this));
  // Header widget and button will be created on first layout/show.
}

IntentiveChatOverlay::~IntentiveChatOverlay() = default;

BEGIN_METADATA(IntentiveChatOverlay)
END_METADATA

void IntentiveChatOverlay::SetPromptText(const std::u16string& text) {
  TrySetPromptText(text, /*attempt=*/0);
}

void IntentiveChatOverlay::TrySetPromptText(const std::u16string& text, int attempt) {
  if (!web_view_)
    return;
  content::WebContents* contents = web_view_->GetWebContents();
  if (!contents)
    return;
  content::RenderFrameHost* rfh = contents->GetPrimaryMainFrame();
  if (!rfh)
    return;

  std::string escaped;
  base::EscapeJSONString(text, /*put_in_quotes=*/true, &escaped);
  std::string js_utf8 = base::StrCat({
      "(function(){ try { var t = ", escaped,
      R"JS(;
function allDocs(win){ var out=[]; function rec(w){ try { out.push(w.document); for (var i=0;i<w.frames.length;i++) rec(w.frames[i]); } catch(_){ } } rec(win); return out; }
function findComposer(doc){ var sels=['textarea','[role=textbox]','div[contenteditable=true]','[data-testid*=composer]','[data-testid*=prompt]','[placeholder*=message i]','[aria-label*=message i]']; for (var i=0;i<sels.length;i++){ try{ var q=doc.querySelector(sels[i]); if(q) return q; }catch(_){ } } var queue=[doc]; while(queue.length){ var node=queue.shift(); if(!node) continue; var tag=node.tagName?String(node.tagName).toLowerCase():''; if(tag==='textarea' || (node.getAttribute && node.getAttribute('role')==='textbox') || node.isContentEditable===true) return node; if(node.shadowRoot) queue.push(node.shadowRoot); var kids=node.children||node.childNodes; if(kids){ for(var k=0;k<kids.length;k++) queue.push(kids[k]); } }
return null; }
var el = findComposer(document);
if (!el) {
  var docs = allDocs(window);
  for (var d=0; d<docs.length && !el; ++d) { try { el = findComposer(docs[d]); } catch(_){ } }
}
if (!el) {
  // Fallback: click near bottom center to focus the composer, then try insert.
  try {
    var x = Math.floor(window.innerWidth/2);
    var y = Math.floor(window.innerHeight - 30);
    var tnode = document.elementFromPoint(x, y);
    if (tnode && tnode.click) tnode.click();
  } catch(_){ }
  var ae = document.activeElement;
  if (ae && (ae.isContentEditable===true || (ae.tagName && ae.tagName.toLowerCase()==='textarea'))) {
    if ('value' in ae) { ae.value = t; } else { ae.textContent = t; }
    try { var ev1=document.createEvent('Event'); ev1.initEvent('input',true,true); ae.dispatchEvent(ev1);} catch(_){ }
    try { var ev2=document.createEvent('Event'); ev2.initEvent('change',true,true); ae.dispatchEvent(ev2);} catch(_){ }
    try { var ev3=document.createEvent('KeyboardEvent'); ev3.initEvent('keyup',true,true); ae.dispatchEvent(ev3);} catch(_){ }
    return true;
  }
  try { document.execCommand('insertText', false, t); return true; } catch(_){ }
  return false;
}
if (typeof el.scrollIntoView === 'function') el.scrollIntoView({block:'nearest'});
if (typeof el.focus === 'function') el.focus();
function setVal(node, val) {
  if ('value' in node) {
    node.value = val;
  } else if ('textContent' in node) {
    node.textContent = val;
  }
}
setVal(el, t);
try { var ev1 = document.createEvent('Event'); ev1.initEvent('input', true, true); el.dispatchEvent(ev1); } catch(_){ }
try { var ev2 = document.createEvent('Event'); ev2.initEvent('change', true, true); el.dispatchEvent(ev2); } catch(_){ }
try { var ev3 = document.createEvent('KeyboardEvent'); ev3.initEvent('keyup', true, true); el.dispatchEvent(ev3); } catch(_){ }
return true; } catch(e) { return false; } })();)JS"
  });
  std::u16string js = base::UTF8ToUTF16(js_utf8);
  auto success = std::make_shared<bool>(false);
  auto remaining = std::make_shared<int>(0);
  contents->ForEachRenderFrameHost([&](content::RenderFrameHost* frame) {
    (*remaining)++;
    frame->ExecuteJavaScriptInIsolatedWorld(
        js,
        base::BindOnce(
            [](std::shared_ptr<bool> success,
               std::shared_ptr<int> remaining,
               base::WeakPtr<IntentiveChatOverlay> self,
               std::u16string text,
               int attempt,
               base::Value result) {
              if (result.is_bool() && result.GetBool())
                *success = true;
              if (--(*remaining) == 0 && self) {
                const int kMaxAttempts = 60;
                if (!*success && attempt < kMaxAttempts) {
                  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
                      FROM_HERE,
                      base::BindOnce(&IntentiveChatOverlay::TrySetPromptText,
                                     self, std::move(text), attempt + 1),
                      base::Milliseconds(500));
                }
              }
            },
            success, remaining, weak_ptr_factory_.GetWeakPtr(), text, attempt),
        content::ISOLATED_WORLD_ID_CONTENT_END);
  });
}

void IntentiveChatOverlay::SendPrompt() {
  TrySendPrompt(/*attempt=*/0);
}

void IntentiveChatOverlay::TrySendPrompt(int attempt) {
  if (!web_view_)
    return;
  content::WebContents* contents = web_view_->GetWebContents();
  if (!contents)
    return;
  content::RenderFrameHost* rfh = contents->GetPrimaryMainFrame();
  if (!rfh)
    return;

  const std::u16string js = uR"JS(((function(){ try {
    function allDocs(win){
      var out=[]; function rec(w){ try { out.push(w.document); for (var i=0;i<w.frames.length;i++) rec(w.frames[i]); } catch(_){ } }
      rec(win); return out;
    }
    function findComposer(doc){
      var root = doc; var queue=[root];
      while(queue.length){
        var node = queue.shift();
        if(!node) continue;
        var tag = node.tagName ? String(node.tagName).toLowerCase() : '';
        if (tag==='textarea' || (node.getAttribute && node.getAttribute('role')==='textbox') || node.isContentEditable===true)
          return node;
        if (node.shadowRoot) queue.push(node.shadowRoot);
        var kids = node.children || node.childNodes;
        if (kids) { for (var i=0;i<kids.length;i++) queue.push(kids[i]); }
      }
      return null;
    }
    var el = document.querySelector('textarea, [role=textbox], div[contenteditable=true]');
    if (!el) {
      var docs = allDocs(window);
      for (var d=0; d<docs.length && !el; ++d) {
        try { el = docs[d].querySelector('textarea, [role=textbox], div[contenteditable=true]') || findComposer(docs[d]); } catch(_){ }
      }
    }
    if (!el) return false;
    var val = ('value' in el) ? el.value : (el.textContent || '');
    if (!val || String(val).trim().length === 0) return false;
    var btn = document.querySelector('[data-testid*=send], button[aria-label*="Send" i], button[type=submit]');
    if (btn) { btn.click(); return true; }
    if (el.focus) el.focus();
    try {
      var kd = new KeyboardEvent('keydown', {key:'Enter', code:'Enter', which:13, keyCode:13, bubbles:true, cancelable:true});
      el.dispatchEvent(kd);
      var ku = new KeyboardEvent('keyup', {key:'Enter', code:'Enter', which:13, keyCode:13, bubbles:true, cancelable:true});
      el.dispatchEvent(ku);
    } catch(_){ }
    btn = document.querySelector('[data-testid*=send], button[aria-label*="Send" i], button[type=submit]');
    if (btn) { btn.click(); return true; }
    return false;
  } catch(e) { return false; } })())JS";
  auto success2 = std::make_shared<bool>(false);
  auto remaining2 = std::make_shared<int>(0);
  contents->ForEachRenderFrameHost([&](content::RenderFrameHost* frame) {
    (*remaining2)++;
    frame->ExecuteJavaScriptInIsolatedWorld(
        js,
        base::BindOnce(
            [](std::shared_ptr<bool> success,
               std::shared_ptr<int> remaining,
               base::WeakPtr<IntentiveChatOverlay> self,
               int attempt,
               base::Value result) {
              if (result.is_bool() && result.GetBool())
                *success = true;
              if (--(*remaining) == 0 && self) {
                const int kMaxAttempts = 40;
                if (!*success && attempt < kMaxAttempts) {
                  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
                      FROM_HERE,
                      base::BindOnce(&IntentiveChatOverlay::TrySendPrompt, self,
                                     attempt + 1),
                      base::Milliseconds(500));
                }
              }
            },
            success2, remaining2, weak_ptr_factory_.GetWeakPtr(), attempt),
        content::ISOLATED_WORLD_ID_CONTENT_END);
  });
}

void IntentiveChatOverlay::OnSendPageToAI() {
  if (!host_parent_view_)
    return;

  // Find the active page WebContents under the host container.
  std::function<views::WebView*(views::View*)> find_webview;
  find_webview = [&find_webview](views::View* root) -> views::WebView* {
    if (!root)
      return nullptr;
    if (auto* wv = views::AsViewClass<views::WebView>(root))
      return wv;
    for (views::View* child : root->children()) {
      if (auto* found = find_webview(child))
        return found;
    }
    return nullptr;
  };

  views::WebView* page_wv = find_webview(host_parent_view_);
  if (!page_wv)
    return;
  content::WebContents* tab = page_wv->GetWebContents();
  if (!tab)
    return;
  content::RenderFrameHost* rfh = tab->GetPrimaryMainFrame();
  if (!rfh)
    return;


  // Collector JS (see requirements).
  const std::u16string js = uR"JS((function () {
    function safe(s) { return (s == null) ? '' : String(s); }

    function getMeta(doc, name, prop) {
      try {
        if (name) {
          var m = doc.querySelector('meta[name="' + name + '"]');
          if (m && m.content) return String(m.content);
        }
        if (prop) {
          var p = doc.querySelector('meta[property="' + prop + '"]');
          if (p && p.content) return String(p.content);
        }
      } catch (_) {}
      return '';
    }

    function shouldSkipNode(node) {
      var el = node.parentElement || node;
      while (el) {
        if (el.closest && el.closest('script,style,noscript,template'))
          return true;
        if (el.hasAttribute && (el.hasAttribute('hidden') || el.getAttribute('aria-hidden') === 'true'))
          return true;
        el = el.parentElement;
      }
      return false;
    }

    function collectText(doc, maxChars) {
      var out = [];
      var total = 0;
      var root = doc && (doc.body || doc.documentElement || doc);
      if (!root) return '';
      try {
        var tw = doc.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
          acceptNode: function (n) {
            if (!n || !n.textContent) return NodeFilter.FILTER_REJECT;
            if (shouldSkipNode(n)) return NodeFilter.FILTER_REJECT;
            var t = n.textContent.replace(/\s+/g, ' ').trim();
            if (!t) return NodeFilter.FILTER_REJECT;
            return NodeFilter.FILTER_ACCEPT;
          }
        }, false);
        var node;
        while ((node = tw.nextNode())) {
          var t = node.textContent.replace(/\s+/g, ' ').trim();
          if (!t) continue;
          if (total + t.length + 1 > maxChars) {
            var remain = Math.max(0, maxChars - total);
            if (remain > 0) {
              out.push(t.slice(0, remain));
              total += remain;
            }
            break;
          }
          out.push(t);
          total += t.length + 1;
        }
      } catch (_) {}
      return out.join(' ');
    }

    function collectDoc(doc, maxChars) {
      var o = {};
      try {
        o.title = safe(doc && doc.title);
        o.url = safe(doc && doc.location && doc.location.href);
        o.lang = safe(doc && doc.documentElement && doc.documentElement.lang);
        var sel = '';
        try { sel = String((doc && doc.getSelection && doc.getSelection().toString()) || ''); } catch (_) {}
        o.selection = sel;

        o.meta = {
          description: getMeta(doc, 'description', null),
          ogTitle: getMeta(doc, null, 'og:title'),
          ogDesc: getMeta(doc, null, 'og:description'),
        };

        o.text = collectText(doc, maxChars);
      } catch (_) {
        o = { title: '', url: '', lang: '', selection: '', meta: {description:'', ogTitle:'', ogDesc:''}, text: '' };
      }
      return o;
    }

    var main = collectDoc(document, 100000);

    var frames = [];
    try {
      var ifrs = document.getElementsByTagName('iframe');
      for (var i = 0; i < ifrs.length; ++i) {
        try {
          var d = ifrs[i].contentDocument;
          if (d) {
            var f = collectDoc(d, 20000);
            frames.push(f);
          }
        } catch (_) { /* cross-origin; skip */ }
      }
    } catch (_) { frames = []; }

    main.frames = frames;
    return main;
  })())JS";

  // Fallback title/url from tab; prefer JS values when present.
  const std::u16string tab_title = tab->GetTitle();
  const GURL tab_gurl = tab->GetURL();

  // UTF-8 safe byte trim helper.
  auto utf8_safe_trim = [](const std::string& utf8, size_t max_bytes) -> std::string {
    if (utf8.size() <= max_bytes)
      return utf8;
    size_t end = std::min(max_bytes, utf8.size());
    while (end > 0 && (static_cast<unsigned char>(utf8[end - 1]) & 0xC0) == 0x80)
      --end;
    if (end == 0) return std::string();
    return utf8.substr(0, end);
  };

  rfh->ExecuteJavaScriptInIsolatedWorld(
      js,
      base::BindOnce(
          [](base::WeakPtr<IntentiveChatOverlay> self,
             std::u16string tab_title, std::u16string tab_url16,
             std::function<std::string(const std::string&, size_t)> trim_fn,
             base::Value result) {
            if (!self)
              return;

            std::u16string title_u16 = std::move(tab_title);
            std::u16string url_u16 = std::move(tab_url16);
            std::u16string lang_u16;
            std::u16string selection_u16;
            std::u16string meta_desc_u16;
            std::u16string og_title_u16;
            std::u16string og_desc_u16;
            std::u16string text_u16;
            std::vector<std::tuple<std::u16string, std::u16string, std::u16string>> frames;  // url, title, text

            if (result.is_dict()) {
              const auto& dict = result.GetDict();
              if (const std::string* t = dict.FindString("title")) if (!t->empty()) title_u16 = base::UTF8ToUTF16(*t);
              if (const std::string* u = dict.FindString("url")) if (!u->empty()) url_u16 = base::UTF8ToUTF16(*u);
              if (const std::string* l = dict.FindString("lang")) lang_u16 = base::UTF8ToUTF16(*l);
              if (const std::string* s = dict.FindString("selection")) selection_u16 = base::UTF8ToUTF16(*s);
              if (const std::string* tx = dict.FindString("text")) text_u16 = base::UTF8ToUTF16(*tx);

              if (const base::Value::Dict* meta = dict.FindDict("meta")) {
                if (const std::string* md = meta->FindString("description")) meta_desc_u16 = base::UTF8ToUTF16(*md);
                if (const std::string* ot = meta->FindString("ogTitle")) og_title_u16 = base::UTF8ToUTF16(*ot);
                if (const std::string* od = meta->FindString("ogDesc")) og_desc_u16 = base::UTF8ToUTF16(*od);
              }

              if (const base::Value::List* fr = dict.FindList("frames")) {
                for (const auto& v : *fr) {
                  if (!v.is_dict()) continue;
                  const auto& fdict = v.GetDict();
                  std::u16string furl, ftitle, ftext;
                  if (const std::string* fu = fdict.FindString("url")) furl = base::UTF8ToUTF16(*fu);
                  if (const std::string* ft = fdict.FindString("title")) ftitle = base::UTF8ToUTF16(*ft);
                  if (const std::string* fx = fdict.FindString("text")) ftext = base::UTF8ToUTF16(*fx);
                  frames.emplace_back(std::move(furl), std::move(ftitle), std::move(ftext));
                }
              }
            }

            std::u16string prompt =
                u"You are being provided content of a webpage.\n"
                u"Use this page as the main context for answering the user’s next questions.\n"
                u"If unrelated, answer normally.\n\n"
                u"--- PAGE CONTENT START ---\n";

            prompt.append(u"Title: ");
            prompt.append(title_u16);
            prompt.append(u"\nURL: ");
            prompt.append(url_u16);
            prompt.append(u"\n");

            if (!lang_u16.empty()) {
              prompt.append(u"Language: ");
              prompt.append(lang_u16);
              prompt.append(u"\n");
            }

            if (!selection_u16.empty()) {
              prompt.append(u"\nSelection:\n");
              prompt.append(selection_u16);
              prompt.append(u"\n");
            }

            if (!meta_desc_u16.empty() || !og_title_u16.empty() || !og_desc_u16.empty()) {
              prompt.append(u"\nMeta:\n");
              if (!meta_desc_u16.empty()) { prompt.append(u"- description: "); prompt.append(meta_desc_u16); prompt.append(u"\n"); }
              if (!og_title_u16.empty()) { prompt.append(u"- og:title: "); prompt.append(og_title_u16); prompt.append(u"\n"); }
              if (!og_desc_u16.empty()) { prompt.append(u"- og:description: "); prompt.append(og_desc_u16); prompt.append(u"\n"); }
            }

            if (!text_u16.empty()) {
              prompt.append(u"\n");
              prompt.append(text_u16);
              prompt.append(u"\n");
            }

            for (const auto& tup : frames) {
              const std::u16string& furl = std::get<0>(tup);
              const std::u16string& ftitle = std::get<1>(tup);
              const std::u16string& ftext = std::get<2>(tup);
              prompt.append(u"\n[Frame]\n");
              if (!ftitle.empty()) { prompt.append(u"Title: "); prompt.append(ftitle); prompt.append(u"\n"); }
              if (!furl.empty()) { prompt.append(u"URL: "); prompt.append(furl); prompt.append(u"\n"); }
              if (!ftext.empty()) { prompt.append(ftext); prompt.append(u"\n"); }
            }

            prompt.append(u"--- PAGE CONTENT END ---");

            std::string prompt_utf8 = base::UTF16ToUTF8(prompt);
            prompt_utf8 = trim_fn(prompt_utf8, 50000u);
            std::u16string final_u16 = base::UTF8ToUTF16(prompt_utf8);

            self->SetPromptText(final_u16);  // Do NOT auto-send.
          },
          weak_ptr_factory_.GetWeakPtr(),
          tab->GetTitle(), base::UTF8ToUTF16(tab_gurl.spec()),
          std::function<std::string(const std::string&, size_t)>(utf8_safe_trim)),
      content::ISOLATED_WORLD_ID_CONTENT_END);
}

void IntentiveChatOverlay::Layout(PassKey) {
  LayoutSuperclass<views::View>(this);
  if (!web_view_)
    return;
  const gfx::Rect bounds = GetContentsBounds();
  web_view_->SetBoundsRect(bounds);
}

void IntentiveChatOverlay::SendPageToAI() {
  OnSendPageToAI();
}
