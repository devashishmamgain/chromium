// Minimal, fast, no frameworks. Handles UI, drag-drop, search, overflow menu, keyboard nav, etc.

let items = [];
let sections = [];
let collapse = {};
let searchTerm = '';
let dragItem = null;
let dragOverSection = null;
let dragOverIndex = null;
let toastTimeout = null;

// DOM refs
const $sections = document.getElementById('sections');
const $search = document.getElementById('search');
const $addCurrent = document.getElementById('add-current');
const $empty = document.getElementById('empty');
const $quickAdd = document.getElementById('quick-add');
const $toast = document.getElementById('toast');

// Debounce helper
function debounce(fn, ms) {
  let timer;
  return (...args) => {
    clearTimeout(timer);
    timer = setTimeout(() => fn(...args), ms);
  };
}

// Toast
function showToast(msg) {
  $toast.textContent = msg;
  $toast.style.display = 'block';
  clearTimeout(toastTimeout);
  toastTimeout = setTimeout(() => { $toast.style.display = 'none'; }, 1800);
}

// Load items & sections (stub for now)
function loadData() {
  // TODO: Replace with backend call
  items = [];
  sections = ['Default'];
  collapse = {};
  render();
}

// Save collapse state (stub)
function saveCollapse() {
  // TODO: Replace with backend call
}

// Search filter (fuzzy)
function filterItems() {
  if (!searchTerm) return items;
  const term = searchTerm.toLowerCase();
  return items.filter(i =>
    i.title.toLowerCase().includes(term) ||
    i.url.toLowerCase().includes(term)
  );
}

// Render UI
function render() {
  $sections.innerHTML = '';
  let filtered = filterItems();
  let hasItems = filtered.length > 0;
  $empty.style.display = hasItems ? 'none' : 'block';

  sections.forEach(section => {
    let secItems = filtered.filter(i => i.section === section);
    let collapsed = collapse[section];
    // Section header
    let secDiv = document.createElement('div');
    secDiv.className = 'section';
    let header = document.createElement('div');
    header.className = 'section-header' + (collapsed ? ' collapsed' : '');
    header.innerHTML = `<span class="arrow">▼</span> <span>${section}</span>`;
    header.onclick = () => {
      collapse[section] = !collapse[section];
      saveCollapse();
      render();
    };
    secDiv.appendChild(header);

    // Items
    let itemsDiv = document.createElement('div');
    itemsDiv.className = 'section-items';
    itemsDiv.style.display = collapsed ? 'none' : '';
    secItems.forEach((item, idx) => {
      let itemDiv = renderItem(item, section, idx, secItems.length);
      itemsDiv.appendChild(itemDiv);
    });
    secDiv.appendChild(itemsDiv);
    $sections.appendChild(secDiv);
  });
}

// Render single item (stub)
function renderItem(item, section, idx, secLen) {
  let div = document.createElement('div');
  div.className = 'item';
  div.tabIndex = 0;
  div.setAttribute('data-id', item.id);

  // Favicon
  let favicon = document.createElement('img');
  favicon.className = 'favicon';
  favicon.src = 'chrome://favicon/' + item.url;
  favicon.onerror = () => { favicon.src = 'icons/icon16.png'; };
  div.appendChild(favicon);

  // Title
  let title = document.createElement('span');
  title.className = 'title';
  title.textContent = item.title;
  div.appendChild(title);

  // URL
  let url = document.createElement('span');
  url.className = 'url';
  url.textContent = item.url;
  div.appendChild(url);

  // Drag handle
  let dragHandle = document.createElement('span');
  dragHandle.className = 'drag-handle';
  dragHandle.innerHTML = '☰';
  dragHandle.draggable = true;
  dragHandle.ondragstart = e => {
    dragItem = item;
    div.classList.add('dragging');
    e.dataTransfer.effectAllowed = 'move';
  };
  dragHandle.ondragend = e => {
    dragItem = null;
    div.classList.remove('dragging');
    dragOverSection = null;
    dragOverIndex = null;
    render();
  };
  div.appendChild(dragHandle);

  // Overflow menu
  let overflow = document.createElement('span');
  overflow.className = 'overflow';
  let btn = document.createElement('button');
  btn.className = 'overflow-btn';
  btn.innerHTML = '⋮';
  btn.onclick = e => {
    e.stopPropagation();
    showOverflowMenu(item, overflow, section);
  };
  overflow.appendChild(btn);
  div.appendChild(overflow);

  // Click behaviors (stub)
  div.onclick = e => {
    if (e.target.classList.contains('overflow-btn')) return;
    // TODO: Open item logic
  };

  // Keyboard nav (stub)
  div.onkeydown = e => {
    if (e.key === 'Enter') {
      // TODO: Open item logic
    }
    if (e.key === 'Delete') {
      // TODO: Delete item logic
    }
    if (e.key === 'ArrowDown') {
      let next = div.nextSibling;
      if (next) next.focus();
    }
    if (e.key === 'ArrowUp') {
      let prev = div.previousSibling;
      if (prev) prev.focus();
    }
  };

  return div;
}

// Overflow menu (stub)
function showOverflowMenu(item, parent, section) {
  closeOverflowMenus();
  let menu = document.createElement('div');
  menu.className = 'overflow-menu';
  [
    { label: 'Open', fn: () => {/* TODO */} },
    { label: 'Open here', fn: () => {/* TODO */} },
    { label: 'Copy link', fn: () => {/* TODO */} },
    { label: 'Edit', fn: () => {/* TODO */} },
    { label: 'Move to section…', fn: () => {/* TODO */} },
    { label: item.pin ? 'Unpin' : 'Pin', fn: () => {/* TODO */} },
    { label: 'Delete', fn: () => {/* TODO */} }
  ].forEach(opt => {
    let btn = document.createElement('button');
    btn.textContent = opt.label;
    btn.onclick = e => {
      opt.fn();
      closeOverflowMenus();
      setTimeout(loadData, 200);
    };
    menu.appendChild(btn);
  });
  parent.appendChild(menu);
  document.addEventListener('click', closeOverflowMenus, { once: true });
}
function closeOverflowMenus() {
  document.querySelectorAll('.overflow-menu').forEach(m => m.remove());
}

// Search
$search.oninput = debounce(e => {
  searchTerm = $search.value;
  render();
}, 180);

// Add current tab (stub)
$addCurrent.onclick = async () => {
  // TODO: Add current tab logic
  showToast('Added current tab!');
  setTimeout(loadData, 200);
};

// Quick Add (stub)
$quickAdd.onclick = async () => {
  let text = prompt('Paste URLs (one per line):');
  if (!text) return;
  // TODO: Add quick add logic
  showToast('Added links!');
  setTimeout(loadData, 200);
};

// Keyboard nav for search
$search.onkeydown = e => {
  if (e.key === 'ArrowDown') {
    let first = $sections.querySelector('.item');
    if (first) first.focus();
  }
};

// Initial load
loadData();