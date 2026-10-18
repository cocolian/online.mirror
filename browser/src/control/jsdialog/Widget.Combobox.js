/* -*- js-indent-level: 8 -*- */
/*
 * Copyright the Collabora Online contributors.
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * JSDialog.Combobox - combobox widget with support for custom renders of entries
 *
 * Example JSON:
 * {
 *     id: 'id',
 *     type: 'combobox',
 *     text: 'some text',
 *     entries: [ 'A', 'B', 'C' ],
 *     customEntryRenderer: true
 * }
 *
 * customEntryRenderer - specifies if entries have custom content which is rendered by the core
 */

/* global JSDialog app _ $ */

// Presentation names never replace the canonical name sent to the engine.
JSDialog.getOfficeFontInfo = function (name) {
	var labels = {
		FangSong_GB2312: '仿宋_GB2312', KaiTi_GB2312: '楷体_GB2312',
		SimHei: '黑体', SimSun: '宋体', NSimSun: '新宋体',
		'Source Han Serif SC': '思源宋体', 'Source Han Sans SC': '思源黑体',
		'WenQuanYi Zen Hei': '文泉驿正黑', 'WenQuanYi Zen Hei Mono': '文泉驿等宽正黑',
		'WenQuanYi Zen Hei Sharp': '文泉驿点阵正黑',
		C059: 'C059（世纪教科书风格）', P052: 'P052（帕拉蒂诺风格）',
		Z003: 'Z003（西文书法体）', D050000L: 'D050000L（装饰符号）',
		'OpenSymbol': 'OpenSymbol（符号）', 'Standard Symbols PS': 'Standard Symbols PS（符号）'
	};
	var display = labels[name] || name;
	var noto = /^Noto (Sans Mono|Sans|Serif) CJK (SC|TC|HK|JP|KR)$/.exec(name);
	if (noto) display = 'Noto ' + ({ 'Sans Mono': '等宽黑体', Sans: '黑体', Serif: '宋体' })[noto[1]] + ' ' + noto[2];
	var group = 9;
	if (/FangSong|仿宋/i.test(name)) group = 0;
	else if (/KaiTi|楷体/i.test(name)) group = 1;
	else if (/Mono|等宽/i.test(name)) group = 6;
	else if (/Source Han Serif|Noto Serif CJK|SimSun|宋体/.test(name)) group = 2;
	else if (/Source Han Sans|Noto Sans CJK|SimHei|WenQuanYi|黑体|文泉驿/.test(name)) group = 3;
	else if (/C059|P052|Bookman|Roman|Serif|Century|Palatino/i.test(name)) group = 4;
	else if (/Sans|Gothic/i.test(name)) group = 5;
	else if (/Z003|Chancery|Script|Hand/i.test(name)) group = 7;
	else if (/Symbol|D050000L/i.test(name)) group = 8;
	var groups = ['仿宋', '楷体', '宋体 / 东亚衬线', '黑体 / 东亚无衬线', '西文衬线', '西文无衬线', '等宽字体', '书法与装饰', '符号字体', '其他字体'];
	return { display: display, group: group, groupName: groups[group] };
};

JSDialog.comboboxEntry = function (parentContainer, data, builder) {
	var fontInfo = data.comboboxId === 'fontnamecombobox' ? JSDialog.getOfficeFontInfo(data.text) : null;
	if (fontInfo && (data.class || '').indexOf('office-font-group-start') >= 0) {
		var heading = window.L.DomUtil.create('div', 'office-font-group', parentContainer);
		heading.textContent = fontInfo.groupName;
		heading.dataset.fontGroup = String(fontInfo.group);
		heading.setAttribute('aria-hidden', 'true');
	}
	var entry = window.L.DomUtil.create('div', 'ui-combobox-entry ' + builder.options.cssClass, parentContainer);
	entry.id = data.id;
	if (data.class)
		entry.classList.add.apply(entry.classList, data.class.split(' '));
	// An icon-only entry (no text) centers its image and shrinks to the
	// icon width instead of leaving a wide empty row.
	if (!data.text)
		window.L.DomUtil.addClass(entry, 'ui-combobox-notext');
	entry.setAttribute('role', 'option');
	entry.setAttribute('tabindex', '-1');
	entry.setAttribute('data-filter-text', (data.text + ' ' + (fontInfo ? fontInfo.display + ' ' + fontInfo.groupName : '')).toLowerCase());
	if (fontInfo) {
		entry.dataset.fontGroup = String(fontInfo.group);
		entry.setAttribute('aria-label', fontInfo.display);
		entry.title = data.text;
	}

	// A disabled entry is dimmed, announced as disabled, and kept out of
	// keyboard navigation: the disabled attribute is what excludes it from the
	// focusable-option lookup.
	var disabled = data.enabled === false;
	if (disabled) {
		window.L.DomUtil.addClass(entry, 'disabled');
		entry.setAttribute('disabled', '');
		entry.setAttribute('aria-disabled', 'true');
	}

	if (data.hasSubMenu)
		window.L.DomUtil.addClass(entry, 'ui-has-menu');

	if (data.w2icon) {
		// FIXME: DEPRECATED, this is legacy way to setup icon based on CSS class
		window.L.DomUtil.create('div', 'w2ui-icon ui-combobox-icon ' + data.w2icon, entry);
	}

	if (data.icon) {
		var icon = window.L.DomUtil.create('img', 'ui-combobox-icon', entry);
		icon.alt = '';
		builder._isStringCloseToURL(data.icon) ? icon.src = data.icon : app.LOUtil.setImage(icon,  app.LOUtil.getIconNameOfCommand(data.icon), builder.map, true);
		if (data.hasSubMenu) {
			window.L.DomUtil.addClass(entry, 'ui-has-img');
		}
	}

	if (data.hint) {
		entry.title = data.hint;
	}

	var content = window.L.DomUtil.create('span', '', entry);
	content.innerText = fontInfo ? fontInfo.display : data.text;

	if (data.shortcut) {
		var shortcut = window.L.DomUtil.create('span', 'shortcut', entry);
		shortcut.innerText = data.shortcut;
	}

    if (data.selected) {
        entry.setAttribute('aria-selected', 'true');
		window.L.DomUtil.addClass(entry, 'selected');
    } else {
        entry.setAttribute('aria-selected', 'false');
    }

	if (data.checked)
		window.L.DomUtil.addClass(entry, 'checked');
	else if (data.checked !== undefined)
		window.L.DomUtil.addClass(entry, 'notchecked');

	if (data.customRenderer) {
		// the text is replaced by the rendered preview, so the entry needs to
		// carry its name for the screen reader on its own
		if (data.text)
			entry.setAttribute('aria-label', data.text);
		JSDialog.OnDemandRenderer(builder, data.comboboxId, 'combobox', data.pos, content, entry, data.text);
	}

	var entryData = data.pos + ';' + data.text;

	var clickFunction = function () {
		if (disabled)
			return;
		builder.callback('combobox', 'selected', {id: data.comboboxId}, entryData, builder);
	};

	entry.addEventListener('click', clickFunction);
	entry.addEventListener('keydown', function (event) {
        if (event.key === 'Enter' || event.key === ' ') {
			clickFunction();
			event.preventDefault();
		} else if (event.key === 'Tab') {
			JSDialog.CloseDropdown(data.comboboxId);
			event.preventDefault();
		}
	});

	entry.addEventListener('mouseenter', function () {
		const grid = entry.parentElement;
		if (!grid)
			return;
		const openEntries = grid.querySelectorAll('.has-open-submenu');
		for (let i = 0; i < openEntries.length; i++) {
			if (openEntries[i] !== entry && openEntries[i].getAttribute('aria-expanded') !== 'true')
				openEntries[i].classList.remove('has-open-submenu');
		}
	});

	if (data.hasSubMenu) {
		entry.setAttribute('aria-haspopup', true);
		entry.setAttribute('aria-expanded', false);
		const onDropDown = function(open) {
			entry.setAttribute('aria-expanded', open);
			if (open)
				entry.classList.add('has-open-submenu');
			else
				entry.classList.remove('has-open-submenu');
		};
		entry._onDropDown = onDropDown;
		parentContainer._onDropDown = onDropDown;

		entry.addEventListener('mouseenter', function () {
			builder.callback('combobox', 'showsubmenu', {id: data.comboboxId}, entryData, builder);
		});
	} else {
		entry.addEventListener('mouseenter', function () {
		builder.callback('combobox', 'hideothersubmenu', {id: data.comboboxId}, entryData, builder);
		});
	}

	return false;
};

JSDialog.mobileComboboxEntry = function(parentContainer, data, builder) {
	var comboboxEntry = window.L.DomUtil.create('p', builder.options.cssClass + ' .ui-mobile-combobox-entry', parentContainer);
	comboboxEntry.textContent = builder._cleanText(data.text);

	comboboxEntry.parent = data.parent;

	if (data.style && data.style.length)
		window.L.DomUtil.addClass(comboboxEntry, data.style);

	comboboxEntry.addEventListener('click', function () {
		if (builder.wizard)
			builder.wizard.goLevelUp();
		builder.callback('combobox', 'selected', comboboxEntry.parent, data.pos + ';' + comboboxEntry.textContent, builder);
	});

	return false;
};

JSDialog.mobileCombobox = function (parentContainer, data, builder) {
	var container = window.L.DomUtil.create('div', 'ui-explorable-entry level-' + builder._currentDepth + ' ' + builder.options.cssClass + ' ui-widget', parentContainer);
	if (data && data.id)
		container.id = data.id;

	var sectionTitle = window.L.DomUtil.create('div', 'ui-header level-' + builder._currentDepth + ' ' + builder.options.cssClass + ' ui-widget', container);
	$(sectionTitle).css('justify-content', 'space-between');

	var leftDiv = window.L.DomUtil.create('div', 'ui-header-left combobox', sectionTitle);

	var editCallback = function(value) {
		builder.callback('combobox', 'change', data, value, builder);
	};
	builder._controlHandlers['edit'](leftDiv, data, builder, editCallback);

	var rightDiv = window.L.DomUtil.create('div', 'ui-header-right', sectionTitle);

	var arrowSpan = window.L.DomUtil.create('span', 'sub-menu-arrow', rightDiv);
	arrowSpan.textContent = '>';

	var contentDiv = window.L.DomUtil.create('div', 'ui-content level-' + builder._currentDepth + ' ' + builder.options.cssClass, container);
	contentDiv.title = data.text;

	var entries = [];
	if (data.entries) {
		for (var index in data.entries) {
			var style = 'ui-combobox-text';
			if ((data.selectedEntries && index == data.selectedEntries[0])
				|| data.entries[index] == data.text) {
				style += ' selected';
			}

			var entry = { type: 'comboboxentry', text: data.entries[index], pos: index, parent: data, style: style };
			entries.push(entry);
		}
	}

	var contentNode = {type: 'container', children: entries};

	builder._currentDepth++;
	builder.build(contentDiv, [contentNode]);
	builder._currentDepth--;

	if (!data.nosubmenu)
	{
		$(contentDiv).hide();
		if (builder.wizard) {
			$(container).click(function(event, data) {
				builder.wizard.goLevelDown(container, data);
				if (contentNode && contentNode.onshow)
					contentNode.onshow();
			});
		} else {
			window.app.console.debug('Builder used outside of mobile wizard: please implement the click handler');
		}
	}
	else
		$(container).hide();

	container.onSelect = function (pos) {
		var nodeEntries = contentDiv.querySelectorAll('.ui-mobile-combobox-entry');
		for (var i = 0; i < nodeEntries.length; i++)
			window.L.DomUtil.removeClass(nodeEntries[i], 'selected');
		if (nodeEntries[pos])
			window.L.DomUtil.addClass(nodeEntries[pos], 'selected');
	};

	container.onUnSelect = function (pos) {
		var nodeEntries = contentDiv.querySelectorAll('.ui-mobile-combobox-entry');
		if (nodeEntries[pos])
			window.L.DomUtil.removeClass(nodeEntries[pos], 'selected');
	};

	container.onSetText = function (text) {
		var input = leftDiv.querySelector('input.ui-edit');
		if (input && document.activeElement !== input)
			input.value = text;
	};
};

function _extractPos(selectCommandData) {
	return selectCommandData.substr(0, selectCommandData.indexOf(';'));
}

function _extractText(selectCommandData) {
	return selectCommandData.substr(selectCommandData.indexOf(';') + 1);
}

JSDialog.combobox = function (parentContainer, data, builder) {
	// Core font previews contain language samples. Plain names keep this list
	// compact, searchable and consistent without changing the selected font.
	if (data.id === 'fontnamecombobox')
		data.customEntryRenderer = false;
	var container = window.L.DomUtil.create('div', 'ui-combobox ' + builder.options.cssClass, parentContainer);
	container.id = data.id;

	if (data.wholeWidthPreview)
		window.L.DomUtil.addClass(container, 'ui-combobox-whole-width-preview');

	var content = window.L.DomUtil.create('input', 'ui-combobox-content ' + builder.options.cssClass, container);
	content.id = data.id + '-input-' + builder.options.suffix;
	content.value = data.id === 'fontnamecombobox' ? JSDialog.getOfficeFontInfo(data.text).display : data.text;
	content.role = 'combobox';
	content.setAttribute('autocomplete', 'off');
	content.setAttribute('aria-autocomplete', 'list');

	JSDialog.SetupA11yLabelForLabelableElement(parentContainer, content, data, builder);

	var dropDownId = JSDialog.CreateDropdownEntriesId(data.id);
	content.setAttribute('aria-expanded', false);

	var button = window.L.DomUtil.create('button', 'ui-combobox-button ' + builder.options.cssClass, container);
	button.setAttribute('aria-expanded', false);

	app.layoutingService.appendLayoutingTask(function () {
		app.layoutingService.appendLayoutingTask(function () {
			const name = JSDialog.GetA11yLabelText(parentContainer, content, data, builder) ||
				(data.aria && data.aria.label ? data.aria.label : '');
			button.setAttribute('aria-label', name
				? _('Open {name}').replace('{name}', name)
				: _('Open list'));
		});
	});

	var arrow = window.L.DomUtil.create('span', builder.options.cssClass + ' ui-listbox-arrow', button);
	arrow.id = 'listbox-arrow-' + data.id;

	container._onDropDown = function (open) {
		content.setAttribute('aria-expanded', open);
		button.setAttribute('aria-expanded', open);

		// Only set aria-controls when dropdown is open to avoid screen reader confusion
		if (open) {
			content.setAttribute('aria-controls', dropDownId);
			button.setAttribute('aria-controls', dropDownId);
		} else {
			content.removeAttribute('aria-controls');
			button.removeAttribute('aria-controls');
		}
	};

	if (data.selectedCount > 0)
		var selectedEntryPos = parseInt(data.selectedEntries[0]);

	// positions which have a separator drawn below them
	var separatorAfter = {};
	var lastPreviewEntry = Infinity;
	for (var s in data.separators) {
		lastPreviewEntry = parseInt(data.separators[s]);
		separatorAfter[lastPreviewEntry] = true;
	}

	// entries the box can hold as its value without offering them in the list
	var isHidden = {};
	for (var h in data.hiddenEntries) {
		isHidden[parseInt(data.hiddenEntries[h])] = true;
	}

	if (data.customEntryRenderer || data.renderSelectedEntry) {
		if (!builder.rendersCache[data.id])
			builder.rendersCache[data.id] = {
				persistent: false,
				images: [],
				dpiScale: window.devicePixelRatio,
			};

		var rendersCache = builder.rendersCache[data.id];
		if (!rendersCache.persistent) {
			var newTexts = data.entries || [];
			var cachedTexts = rendersCache.entryTexts || [];
			var known = Math.max(newTexts.length, cachedTexts.length);
			for (var p = 0; p < known; p++) {
				if (cachedTexts[p] !== newTexts[p])
					delete rendersCache.images[p];
			}
			rendersCache.entryTexts = Array.prototype.slice.call(newTexts);
		}
	}

	// convert to dropdown entries
	var entries = [];
	for (var i in data.entries) {
		var isPreviewEntry =
			data.renderSelectedEntry && parseInt(i) <= lastPreviewEntry;
		entries.push({
			hidden: isHidden[parseInt(i)],
			text: data.entries[i].toString(),
			// keep the name as a tooltip when it is only shown as an icon
			hint: isPreviewEntry ? data.entries[i].toString() : undefined,
			selected: parseInt(i) === selectedEntryPos,
			customRenderer: data.renderSelectedEntry
				? isPreviewEntry
				: data.customEntryRenderer,
			// icon-only rows for line-end arrow pickers
			class: isPreviewEntry
				? data.wholeWidthPreview
					? 'ui-combobox-lineend ui-combobox-lineend-whole'
					: 'ui-combobox-lineend'
				: data.id === 'fontnamecombobox' ? 'office-font-entry' : undefined,
			separatorAfter: separatorAfter[parseInt(i)]
		});
	}

	var sortFontEntries = function () {
		if (data.id !== 'fontnamecombobox') return;
		var commonFonts = ['FangSong_GB2312', '仿宋_GB2312', 'KaiTi_GB2312', '楷体_GB2312', 'SimHei', '黑体', 'Source Han Serif SC', '思源宋体', 'Source Han Sans SC', '思源黑体', 'Noto Serif CJK SC', 'Noto Sans CJK SC', 'WenQuanYi Zen Hei', '文泉驿正黑'];
		entries.forEach(function(entry, index) {
			entry.sourcePos = index;
			entry.fontInfo = JSDialog.getOfficeFontInfo(entry.text);
			entry.displayText = entry.fontInfo.display;
		});
		entries.sort(function(a, b) {
			var aRank = commonFonts.indexOf(a.text);
			var bRank = commonFonts.indexOf(b.text);
			if (aRank < 0) aRank = commonFonts.length;
			if (bRank < 0) bRank = commonFonts.length;
			return a.fontInfo.group - b.fontInfo.group || aRank - bRank || a.displayText.localeCompare(b.displayText, 'zh-CN', { numeric: true, sensitivity: 'base' });
		});
		entries.forEach(function(entry, index) {
			entry.class = 'office-font-entry' + (index === 0 || entries[index - 1].fontInfo.group !== entry.fontInfo.group ? ' office-font-group-start' : '');
		});
	};
	sortFontEntries();

	var canonicalFontName = function(value) {
		if (data.id !== 'fontnamecombobox') return value;
		var match = entries.find(function(entry) { return entry.text === value || entry.displayText === value; });
		return match ? match.text : value;
	};

	var resetSelection = function () {
		for (var i in entries) {
			entries[i].selected = false;
		}
	};

	if (data.enabled === false) {
		container.setAttribute('disabled', 'true');
		container.disabled = true;
		content.setAttribute('disabled', 'true');
		content.disabled = true;
		button.setAttribute('disabled', 'true');
		button.disabled = true;
	}

	JSDialog.SynchronizeDisabledState(container, [content, button]);

	// notebookbar a11y requires main element to have click handler for shortcuts to work
	container.addEventListener('click', function () { content.focus(); });

	content.addEventListener('keyup', function (event) {
		// Suggest the first entry that starts with what has been typed so far, with the
		// suggested remainder selected: typing further replaces it, accepting it (Tab,
		// Return, or an arrow past it) keeps it. event.key.length === 1 excludes named
		// keys (Backspace, arrows, Enter, ...), whose native effect on the field is left
		// alone here.
		if (data.entrycompletion !== false && event.key.length === 1) {
			const typed = this.value.substring(0, this.selectionStart);
			const match = typed.length > 0 && entries.find(function (entry) {
				var label = entry.displayText || entry.text;
				return label.length > typed.length && label.toLowerCase().startsWith(typed.toLowerCase());
			});
			if (match) {
				this.value = match.displayText || match.text;
				this.setSelectionRange(typed.length, this.value.length);
			}
		}

		if (event.key === 'Enter') {
			// Accepting a suggested completion commits it outright: the ghost-highlighted
			// remainder is no longer a pending choice, so clear it and leave the cursor
			// at the end, the same as it would be after picking an entry from the list.
			this.setSelectionRange(this.value.length, this.value.length);
		}

		const shouldTriggerChange = data.changeOnEnterOnly ? event.key === 'Enter' : true;
		if (shouldTriggerChange) {
			builder.callback('combobox', 'change', data, canonicalFontName(this.value), builder);
			if (data.focusMapOnEnter && event.key === 'Enter') {
				builder.map.focus();
			}
		}

		// Return is a distinct commit action, sent in addition to any 'change' above:
		// it marks the current text as explicitly confirmed, not merely typed so far.
		if (event.key === 'Enter')
			builder.callback('combobox', 'activate', data, canonicalFontName(this.value), builder);

		resetSelection();
		for (var i in entries) {
			if (entries[i] == this.value || entries[i].text == this.value || entries[i].displayText == this.value) {
				entries[i].selected = true;
				break;
			}
		}
		// check for drop down is in open state or not.
		// If open then we should make focus in entries field for Arrow key navigation
		if (event.key === 'ArrowDown' && builder.map.jsdialog.hasDropdownOpened()) {
			const comboboxEntries = JSDialog.GetDropdown(data.id);
			const selectedElement = comboboxEntries.querySelector(".selected");
			if (selectedElement) {
				selectedElement.focus();
				return;
			}
		}
	});

	var comboboxId = data.id;
	var clickFunction = function () {
		if (container.hasAttribute('disabled'))
			return;

		var parentBuilder = builder;
		var callback = function(objectType, eventType, object, data) {
			// send command with correct WindowId (from parent, not dropdown)
			let result;
			var callbackData = data;
			if (comboboxId === 'fontnamecombobox' && eventType === 'selected') {
				var selectedFont = entries[parseInt(_extractPos(data))];
				if (selectedFont) callbackData = selectedFont.sourcePos + ';' + _extractText(data);
			}
			if (eventType !== 'close')
				result = parentBuilder.callback(objectType, eventType, object, callbackData, parentBuilder);

			// close after selection
			if (eventType === 'selected') {
				container.onSelect(_extractPos(data));
				container.onSetText(_extractText(data));

				// Pass through if the parent callback has already set the focus
				// somewhere that shouldn't be changed by the CloseDropdown, e.g.
				// toolbar font name/size
				JSDialog.CloseDropdown(comboboxId, result === 'focusHandled');
			}

			return true;
		};

		JSDialog.OpenDropdown(data.id, container, entries, callback);

		if (entries.length > 0) {
			// Inject search field after dropdown DOM is created.
			// Double-nest so it runs after setupInitialFocus.
			app.layoutingService.appendLayoutingTask(function () {
				app.layoutingService.appendLayoutingTask(function () {
					var dropdownRoot = JSDialog.GetDropdown(data.id);
					if (!dropdownRoot)
						return;

					var grid = dropdownRoot.querySelector('.ui-grid[role="listbox"]');
					if (!grid || grid.querySelector('.ui-combobox-search-input'))
						return;

					var searchContainer = document.createElement('div');
					searchContainer.className = 'ui-combobox-search-container';

					var searchInput = document.createElement('input');
					searchInput.type = 'search';
					searchInput.className = 'jsdialog ui-edit ui-combobox-search-input';
					searchInput.setAttribute('placeholder', _('Search...'));
					searchInput.setAttribute('aria-label', _('Filter entries'));
					searchInput.setAttribute('autocomplete', 'off');
					searchInput.setAttribute('tabindex', '0');
					searchContainer.appendChild(searchInput);

					grid.insertAdjacentElement('afterbegin', searchContainer);

					searchInput.addEventListener('input', function () {
						var filterText = searchInput.value.trim().toLowerCase();
						var allEntries = grid.querySelectorAll('.ui-combobox-entry');
						allEntries.forEach(function (entry) {
							var text = entry.getAttribute('data-filter-text') || '';
							if (filterText === '' || text.indexOf(filterText) >= 0) {
								window.L.DomUtil.removeClass(entry, 'hidden');
							} else {
								window.L.DomUtil.addClass(entry, 'hidden');
							}
						});
						grid.querySelectorAll('.office-font-group').forEach(function (heading) {
							var visible = Array.from(allEntries).some(function (entry) {
								return entry.dataset.fontGroup === heading.dataset.fontGroup && !entry.classList.contains('hidden');
							});
							heading.classList.toggle('hidden', !visible);
						});
					});

					searchInput.addEventListener('keydown', function (event) {
						if (event.key === 'ArrowDown') {
							var firstVisible = grid.querySelector('.ui-combobox-entry:not(.hidden)');
							if (firstVisible) {
								firstVisible.focus();
								event.preventDefault();
								event.stopPropagation();
							}
						} else if (event.key === 'Enter') {
							var firstVisible = grid.querySelector('.ui-combobox-entry:not(.hidden)');
							if (firstVisible)
								firstVisible.click();
							event.preventDefault();
						} else if (event.key === 'Escape') {
							JSDialog.CloseDropdown(comboboxId);
							content.focus();
							event.preventDefault();
						}
					});

					searchInput.focus();
				});
			});
		}
	};

	button.addEventListener('click', clickFunction);
	button.addEventListener('keypress', function (event) {
		if (event.key === 'Enter' || event.key === ' ')
			clickFunction();
	});

	// The field shows its own text while no picture is there, so the name and
	// the preview never appear at once.
	const showValuePreview = function (preview, text, image) {
		preview.src = image;
		preview.alt = text;
		preview.title = text;
		window.L.DomUtil.addClass(container, 'ui-combobox-rendered-value');
	};

	container.updateRenders = function (pos) {
		if (container._selectedValuePos === parseInt(pos)) {
			var valuePreview = container.querySelector(
				':scope > img.ui-combobox-value-preview');
			var cachedValue =
				builder.rendersCache[data.id] &&
				builder.rendersCache[data.id].images[pos];
			if (valuePreview && cachedValue)
				showValuePreview(valuePreview, entries[pos].text, cachedValue);
		}

		var dropdownRoot = JSDialog.GetDropdown(data.id);
		if (!dropdownRoot)
			return;

		var row = dropdownRoot.querySelector('#' + data.id + '-entry-' + pos);
		if (row) {
			row.replaceChildren();
			var img = window.L.DomUtil.create('img', '', row);
			img.src = builder.rendersCache[data.id].images[pos];
			img.alt = entries[pos].text;
			img.title = entries[pos].text;
		}
	};

	if (data.renderSelectedEntry && data.customEntryRenderer) {
		container._selectedValuePos = -1;
		container._renderSelectedValue = function (pos) {
			container._selectedValuePos =
				pos === undefined || isNaN(pos) ? -1 : parseInt(pos);

			var preview = container.querySelector(
				':scope > .ui-combobox-value-preview');

			var entry = entries[container._selectedValuePos];
			if (container._selectedValuePos < 0 || !entry || !entry.customRenderer) {
				window.L.DomUtil.removeClass(container, 'ui-combobox-rendered-value');
				if (preview)
					window.L.DomUtil.remove(preview);
				return;
			}

			if (!preview)
				preview = window.L.DomUtil.create(
					'img', 'ui-combobox-value-preview', container);

			var cache = builder.rendersCache[data.id];
			if (cache && cache.images[container._selectedValuePos]) {
				showValuePreview(preview, entry.text,
					cache.images[container._selectedValuePos]);
			} else {
				window.L.DomUtil.removeClass(container, 'ui-combobox-rendered-value');
				var pendingKey = data.id + ':' + container._selectedValuePos;
				if (!app.pendingOnDemandRenderRequests.has(pendingKey)) {
					app.pendingOnDemandRenderRequests.add(pendingKey);
					app.pendingOnDemandRenders++;
				}
				builder.callback(
					'combobox',
					'render_entry',
					{ id: data.id },
					container._selectedValuePos +
						';' +
						Math.floor(100 * window.devicePixelRatio) +
						';' +
						Math.floor(100 * window.devicePixelRatio),
					builder,
				);
			}
		};

		if (data.selectedCount > 0)
			container._renderSelectedValue(selectedEntryPos);
	}

	container.onSelect = function (pos) {
		resetSelection();
		// -1 is a valid position: it deselects everything
		if (pos >= 0 && entries[pos]) {
			entries[pos].selected = true;
			if (document.activeElement !== content)
				content.value = entries[pos].displayText || entries[pos].text;
		} else if (pos >= 0)
			console.warn('Cannot find entry with pos: "' + pos + '" in "' + data.id + '"');
		if (typeof container._renderSelectedValue === 'function')
			container._renderSelectedValue(pos);
	};

	container.onSetText = function (text) {
		if (typeof container._renderSelectedValue === 'function') {
			var textPos = -1;
			for (var e = 0; e < entries.length; e++) {
				if (entries[e].text === text) { textPos = e; break; }
			}
			container._renderSelectedValue(textPos);
		}
		if (document.activeElement === content)
			return;
		content.value = data.id === 'fontnamecombobox' ? JSDialog.getOfficeFontInfo(text).display : text;
	};

	container.updateEntries = function (newEntries) {
		entries = [];
		for (var i = 0; i < newEntries.length; i++) {
			entries.push({
				text: newEntries[i].toString(),
			class: data.id === 'fontnamecombobox' ? 'office-font-entry' : undefined,
				selected: false,
				customRenderer: data.customEntryRenderer
			});
		}
		sortFontEntries();
		if (JSDialog.GetDropdown(data.id))
			JSDialog.CloseDropdown(data.id);
	};

	// The builder sets the tooltip from the widget data after this returns, so mark the box later.
	if (data.id === 'fontnamecombobox' && builder.map)
		app.layoutingService.appendLayoutingTask(function () {
			builder.map.markFontNameBox(container);
		});

	return false;
};
