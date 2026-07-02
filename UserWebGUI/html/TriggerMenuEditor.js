var TriggerMenuEditor = TriggerMenuEditor || {};

(function() {
	"use strict";

	// {name: docName, versions: [versionStr, ...]}, keyed by docName
	var _docs = {};

	// currently loaded menu: {name, version, json}
	var _currentDoc = null;
	var _dirty = false;

	//=====================================================================================
	TriggerMenuEditor.init = function() {
		Debug.log("TriggerMenuEditor.init() localUrnLid=" + DesktopContent._localUrnLid +
			" localOrigin=" + DesktopContent._localOrigin);
		TriggerMenuEditor.loadVariables();
		TriggerMenuEditor.loadJsonDocuments();
	}; //end init()

	//=====================================================================================
	TriggerMenuEditor.loadVariables = function() {
		DesktopContent.XMLHttpRequest(
			"Request?RequestType=getArtdaqSystemVariables",
			"",
			function(req) {
				if (!req) {
					showStatus("Error loading variables", "error");
					return;
				}

				var dataEl = req.responseXML.getElementsByTagName("DATA")[0];
				if (!dataEl) return;

				var vars = {};
				var children = dataEl.childNodes;
				for (var i = 0; i < children.length; i++) {
					var node = children[i];
					if (node.nodeType !== 1) continue;
					var tagName = node.nodeName || node.tagName;
					if (!tagName || tagName.indexOf("artdaq_") !== 0) continue;

					var varName = tagName.substring(7);
					var value = node.getAttribute("value") || node.textContent || "";
					vars[varName] = value;
				}

				var outputBaseDirInput = document.getElementById("tme-outputBaseDir");
				if (outputBaseDirInput && vars.outputBaseDir !== undefined) {
					outputBaseDirInput.value = vars.outputBaseDir;
				}

				// stash desired selections; applied once the dropdowns are populated
				TriggerMenuEditor._pendingMenuName = vars.triggerMenuName || "";
				TriggerMenuEditor._pendingMenuTag  = vars.triggerMenuTag  || "";

				applyPendingSelections();
			},
			undefined, undefined, true,
			true
		);
	}; //end loadVariables()

	//=====================================================================================
	TriggerMenuEditor.saveVariable = function(key, inputId) {
		var input = document.getElementById(inputId);
		if (!input) return;
		var value = input.value;

		DesktopContent.XMLHttpRequest(
			"Request?RequestType=setArtdaqSystemVariable",
			"key=" + encodeURIComponent(key) +
			"&value=" + encodeURIComponent(value),
			function(req) {
				if (!req) {
					showStatus("Error saving variable", "error");
					return;
				}
				var errStr = DesktopContent.getXMLValue(req, "Error");
				if (errStr) {
					showStatus(errStr, "error");
				} else {
					showStatus("Saved " + key + " = " + value, "success");
				}
			},
			undefined, undefined, true,
			true
		);
	}; //end saveVariable()

	//=====================================================================================
	TriggerMenuEditor.loadJsonDocuments = function() {
		var container = document.getElementById("tme-menu-browser");
		if (!container) return;

		DesktopContent.XMLHttpRequest(
			"Request?RequestType=getJsonDocuments",
			"",
			function(req) {
				if (!req) {
					container.innerHTML = "Error loading documents.";
					return;
				}

				var dataEl = req.responseXML.getElementsByTagName("DATA")[0];
				if (!dataEl) {
					container.innerHTML = "No JSON documents found.";
					return;
				}

				var nameNodes    = dataEl.getElementsByTagName("jsonDoc_name");
				var versionNodes = dataEl.getElementsByTagName("jsonDoc_versions");

				_docs = {};

				if (!nameNodes.length) {
					container.innerHTML = "No JSON documents found.";
					renderMenuNameDropdown();
					return;
				}

				var html = '<table><tr><th>Document Name</th><th>Tag</th></tr>';
				for (var i = 0; i < nameNodes.length; i++) {
					var name = nameNodes[i].getAttribute("value") ||
					           nameNodes[i].textContent || "";
					var versCSV = versionNodes[i] ?
						(versionNodes[i].getAttribute("value") ||
						 versionNodes[i].textContent || "") : "";
					var versions = versCSV ? versCSV.split(",") : [];
					_docs[name] = versions;

					html += '<tr><td>' + escapeHtml(name) + '</td><td>';
					for (var j = 0; j < versions.length; j++) {
						html += '<a onclick="TriggerMenuEditor.loadMenuContent(\'' +
							escapeAttr(name) + '\',\'' + escapeAttr(versions[j]) + '\')">' +
							escapeHtml(versions[j]) + '</a>';
						if (j < versions.length - 1) html += ', ';
					}
					html += '</td></tr>';
				}
				html += '</table>';
				container.innerHTML = html;

				renderMenuNameDropdown();
			},
			undefined, undefined, true,
			true
		);
	}; //end loadJsonDocuments()

	//=====================================================================================
	function renderMenuNameDropdown() {
		var select = document.getElementById("tme-triggerMenuName");
		if (!select) return;

		var names = Object.keys(_docs);
		select.innerHTML = "";
		for (var i = 0; i < names.length; i++) {
			var opt = document.createElement("option");
			opt.value = names[i];
			opt.textContent = names[i];
			select.appendChild(opt);
		}

		applyPendingSelections();
	} //end renderMenuNameDropdown()

	//=====================================================================================
	function applyPendingSelections() {
		var nameSelect = document.getElementById("tme-triggerMenuName");
		if (!nameSelect || !nameSelect.options.length) return;

		if (TriggerMenuEditor._pendingMenuName &&
		    _docs[TriggerMenuEditor._pendingMenuName] !== undefined) {
			nameSelect.value = TriggerMenuEditor._pendingMenuName;
		}

		renderMenuTagDropdown(TriggerMenuEditor._pendingMenuTag);
	} //end applyPendingSelections()

	//=====================================================================================
	TriggerMenuEditor.onMenuNameVarChange = function() {
		renderMenuTagDropdown();
	}; //end onMenuNameVarChange()

	//=====================================================================================
	function renderMenuTagDropdown(preferredVersion) {
		var nameSelect = document.getElementById("tme-triggerMenuName");
		var tagSelect  = document.getElementById("tme-triggerMenuTag");
		if (!nameSelect || !tagSelect) return;

		var selectedName = nameSelect.value;
		var versions = _docs[selectedName] || [];

		tagSelect.innerHTML = "";
		for (var i = 0; i < versions.length; i++) {
			var opt = document.createElement("option");
			opt.value = versions[i];
			opt.textContent = versions[i];
			tagSelect.appendChild(opt);
		}

		if (preferredVersion && versions.indexOf(preferredVersion) !== -1) {
			tagSelect.value = preferredVersion;
		}
	} //end renderMenuTagDropdown()

	//=====================================================================================
	TriggerMenuEditor.loadMenuContent = function(docName, docVersion) {
		if (_dirty && !confirm("Discard unsaved prescale edits?")) return;

		DesktopContent.XMLHttpRequest(
			"Request?RequestType=getJsonDocumentContent" +
			"&docName=" + encodeURIComponent(docName) +
			"&docVersion=" + encodeURIComponent(docVersion),
			"",
			function(req) {
				if (!req) {
					showStatus("Error loading menu content", "error");
					return;
				}
				var errStr = DesktopContent.getXMLValue(req, "Error");
				if (errStr) {
					showStatus(errStr, "error");
					return;
				}

				var content = DesktopContent.getXMLValue(req, "content");
				var json;
				try {
					json = JSON.parse(content);
				} catch (e) {
					showStatus("Failed to parse menu JSON: " + e.message, "error");
					return;
				}

				_currentDoc = { name: docName, version: docVersion, json: json };
				_dirty = false;

				updateCurrentDocLabel();

				renderTriggerPaths(json);
				updateSaveButtonState();
				updateRawJson();

				showStatus("Loaded " + docName + " v" + docVersion, "success");
			},
			undefined, undefined, true,
			true
		);
	}; //end loadMenuContent()

	//=====================================================================================
	function renderTriggerPaths(json) {
		var container = document.getElementById("tme-path-table");
		if (!container) return;

		var paths = json.trigger_paths || {};
		var pathNames = Object.keys(paths);

		if (!pathNames.length) {
			container.innerHTML = "No trigger paths in this menu.";
			return;
		}

		// gather the union of eventModes across all paths, in first-seen order
		var eventModes = [];
		for (var p = 0; p < pathNames.length; p++) {
			var configs = paths[pathNames[p]].eventModeConfig || [];
			for (var c = 0; c < configs.length; c++) {
				if (eventModes.indexOf(configs[c].eventMode) === -1) {
					eventModes.push(configs[c].eventMode);
				}
			}
		}

		var html = '<table><tr><th>Path Name</th><th>Bit</th><th>Enabled</th>';
		for (var e = 0; e < eventModes.length; e++) {
			html += '<th>' + escapeHtml(eventModes[e]) + ' Prescale</th>';
		}
		html += '</tr>';

		for (var i = 0; i < pathNames.length; i++) {
			var name = pathNames[i];
			var entry = paths[name];
			var configs = entry.eventModeConfig || [];

			html += '<tr><td>' + escapeHtml(name) + '</td>' +
				'<td>' + escapeHtml(String(entry.bit)) + '</td>' +
				'<td>' + renderEnabledBadge(name, entry.enabled) + '</td>';

			for (var e2 = 0; e2 < eventModes.length; e2++) {
				var configIdx = -1;
				for (var c2 = 0; c2 < configs.length; c2++) {
					if (configs[c2].eventMode === eventModes[e2]) {
						configIdx = c2;
						break;
					}
				}

				if (configIdx === -1) {
					html += '<td>-</td>';
					continue;
				}

				var cfg = configs[configIdx];
				var streamsStr = (cfg.streams || []).join(", ");

				html += '<td>' +
					'<input type="number" min="-1" title="-1 = off" value="' +
					escapeAttr(String(cfg.prescale)) +
					'" onchange="TriggerMenuEditor.onPrescaleEdit(\'' +
					escapeAttr(name) + '\',' + configIdx + ',this)">' +
					'<div class="tme-streams">' + escapeHtml(streamsStr) + '</div>' +
					'</td>';
			}

			html += '</tr>';
		}
		html += '</table>';
		container.innerHTML = html;
	} //end renderTriggerPaths()

	//=====================================================================================
	function renderEnabledBadge(pathName, enabled) {
		var isOn = !!enabled;
		return '<span class="tme-enabled-badge ' + (isOn ? 'tme-enabled-on' : 'tme-enabled-off') +
			'" title="Click to toggle" onclick="TriggerMenuEditor.onEnabledToggle(\'' +
			escapeAttr(pathName) + '\',this)">' + (isOn ? 'Enabled' : 'Disabled') + '</span>';
	} //end renderEnabledBadge()

	//=====================================================================================
	TriggerMenuEditor.onEnabledToggle = function(pathName, badgeEl) {
		if (!_currentDoc) return;

		var entry = _currentDoc.json.trigger_paths[pathName];
		var newEnabled = entry.enabled ? 0 : 1;
		entry.enabled = newEnabled;

		badgeEl.textContent = newEnabled ? 'Enabled' : 'Disabled';
		badgeEl.classList.toggle('tme-enabled-on', !!newEnabled);
		badgeEl.classList.toggle('tme-enabled-off', !newEnabled);
		badgeEl.classList.add('dirty');

		_dirty = true;
		updateSaveButtonState();
		updateRawJson();
	}; //end onEnabledToggle()

	//=====================================================================================
	TriggerMenuEditor.onPrescaleEdit = function(pathName, eventModeIdx, inputEl) {
		if (!_currentDoc) return;

		var newValue = parseInt(inputEl.value, 10);
		if (isNaN(newValue) || newValue < -1) {
			showStatus("Prescale must be -1 (off) or a non-negative integer.", "error");
			return;
		}

		_currentDoc.json.trigger_paths[pathName].eventModeConfig[eventModeIdx].prescale = newValue;
		inputEl.classList.add("dirty");
		_dirty = true;
		updateSaveButtonState();
		updateRawJson();
	}; //end onPrescaleEdit()

	//=====================================================================================
	function updateSaveButtonState() {
		var btn = document.getElementById("tme-save-btn");
		var table = document.getElementById("tme-path-table");

		var isDirty = !!(_dirty && _currentDoc);

		if (btn) {
			if (isDirty) {
				btn.removeAttribute("disabled");
				btn.classList.remove("disabledLink");
			} else {
				btn.setAttribute("disabled", "disabled");
				btn.classList.add("disabledLink");
			}
		}

		if (table) {
			table.classList.toggle("dirty", isDirty);
		}
	} //end updateSaveButtonState()

	//=====================================================================================
	function updateCurrentDocLabel() {
		var el = document.getElementById("tme-current-doc");
		if (!el) return;

		if (!_currentDoc) {
			el.textContent = "No menu loaded.";
			return;
		}

		el.innerHTML =
			'<span class="tme-doc-label">Name</span>' + escapeHtml(_currentDoc.name) +
			'<span class="tme-doc-sep">|</span>' +
			'<span class="tme-doc-label">Tag</span>' + escapeHtml(_currentDoc.version);
	} //end updateCurrentDocLabel()

	//=====================================================================================
	function updateRawJson() {
		var pre = document.getElementById("tme-raw-json");
		if (!pre) return;
		pre.textContent = _currentDoc ?
			JSON.stringify(_currentDoc.json, null, 2) : "No menu loaded.";
	} //end updateRawJson()

	//=====================================================================================
	TriggerMenuEditor.toggleRawJson = function() {
		var pre = document.getElementById("tme-raw-json");
		var toggle = document.getElementById("tme-raw-json-toggle");
		if (!pre || !toggle) return;

		var expanded = pre.style.display !== "none";
		pre.style.display = expanded ? "none" : "block";
		toggle.innerHTML = (expanded ? "&#9656; " : "&#9662; ") + "Raw JSON";
	}; //end toggleRawJson()

	//=====================================================================================
	TriggerMenuEditor.saveAsNewVersion = function() {
		if (!_currentDoc || !_dirty) return;

		var content = JSON.stringify(_currentDoc.json);

		DesktopContent.XMLHttpRequest(
			"Request?RequestType=saveJsonDocumentContent",
			"docName=" + encodeURIComponent(_currentDoc.name) +
			"&content=" + encodeURIComponent(content),
			function(req) {
				if (!req) {
					showStatus("Error saving menu", "error");
					return;
				}
				var errStr = DesktopContent.getXMLValue(req, "Error");
				if (errStr) {
					showStatus(errStr, "error");
					return;
				}

				var newVersion = DesktopContent.getXMLValue(req, "newVersion");
				showStatus("Saved as version " + newVersion, "success");

				var savedName = _currentDoc.name;
				_dirty = false;

				TriggerMenuEditor.loadJsonDocuments();
				TriggerMenuEditor.loadMenuContent(savedName, newVersion);
			},
			undefined, undefined, true,
			true
		);
	}; //end saveAsNewVersion()

	//=====================================================================================
	function showStatus(message, type) {
		var el = document.getElementById("statusBar");
		if (!el) return;
		el.textContent = message;
		el.className = type || "";
		if (type === "success") {
			setTimeout(function() {
				el.textContent = "";
				el.className = "";
			}, 5000);
		}
	} //end showStatus()

	//=====================================================================================
	function escapeHtml(s) {
		var div = document.createElement("div");
		div.appendChild(document.createTextNode(s));
		return div.innerHTML;
	} //end escapeHtml()

	//=====================================================================================
	function escapeAttr(s) {
		return s.replace(/&/g, "&amp;").replace(/"/g, "&quot;")
		        .replace(/'/g, "&#39;").replace(/</g, "&lt;")
		        .replace(/>/g, "&gt;");
	} //end escapeAttr()

})();
