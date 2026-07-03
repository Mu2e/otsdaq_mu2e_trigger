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
	var ARTDAQ_VARIABLE_FIELDS = [
		{ key: "outputBaseDir",   inputId: "tme-outputBaseDir" },
		{ key: "triggerMenuName", inputId: "tme-triggerMenuName" },
		{ key: "triggerMenuTag",  inputId: "tme-triggerMenuTag" }
	];

	TriggerMenuEditor.saveAllVariables = function() {
		saveVariablesSequentially(ARTDAQ_VARIABLE_FIELDS.slice(), []);
	}; //end saveAllVariables()

	//=====================================================================================
	function saveVariablesSequentially(remaining, errors) {
		if (!remaining.length) {
			if (errors.length)
				showStatus(errors.join(" "), "error");
			else
				showStatus("Saved all variables.", "success");
			return;
		}

		var field = remaining.shift();
		var input = document.getElementById(field.inputId);
		var value = input ? input.value : "";

		DesktopContent.XMLHttpRequest(
			"Request?RequestType=setArtdaqSystemVariable",
			"key=" + encodeURIComponent(field.key) +
			"&value=" + encodeURIComponent(value),
			function(req) {
				if (!req) {
					errors.push("Error saving " + field.key + ".");
				} else {
					var errStr = DesktopContent.getXMLValue(req, "Error");
					if (errStr) errors.push(errStr);
				}
				saveVariablesSequentially(remaining, errors);
			},
			undefined, undefined, true,
			true
		);
	} //end saveVariablesSequentially()

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
			var rowId = "tme-seq-row-" + i;

			html += '<tr><td>' +
				'<a title="Show module sequence" onclick="TriggerMenuEditor.toggleSequence(\'' +
				escapeAttr(name) + '\',\'' + rowId + '\')">' + escapeHtml(name) + '</a>' +
				'</td>' +
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
			html += '<tr id="' + rowId + '" class="tme-seq-row" style="display:none;">' +
				'<td colspan="' + (3 + eventModes.length) + '">' +
				'<div class="tme-seq-content">Loading...</div>' +
				'</td></tr>';
		}
		html += '</table>';
		container.innerHTML = html;
	} //end renderTriggerPaths()

	//=====================================================================================
	var TRIGGER_SEQUENCES_FCL_PATH   = "mu2e-trig-config/core/trigSequences.fcl";
	var TRIGGER_RECO_SEQ_FCL_PATH    = "mu2e-trig-config/core/trigRecoSequences.fcl";
	var TRIGGER_FILTERS_INDEX_PATH   = "mu2e-trig-config/core/trigFilters.fcl";
	var TRIGGER_PRODUCERS_INDEX_PATH = "mu2e-trig-config/core/trigProducers.fcl";
	var MU2E_TRIG_CONFIG_INCLUDE_RE  = /#include\s+"(mu2e-trig-config\/[^"]+)"/g;
	var CODE_EDITOR_SUPERVISOR_CLASS = "CodeEditorSupervisor";

	var _codeEditorUrn;                 // cached once resolved
	var _fclFileCache = {};             // relativePath -> file content string
	var _filterProducerFilesPromise;    // cached list of filters/producers fcl paths

	TriggerMenuEditor.toggleSequence = function(pathName, rowId) {
		var row = document.getElementById(rowId);
		if (!row) return;

		if (row.style.display !== "none") {
			row.style.display = "none";
			return;
		}
		row.style.display = "table-row";

		var contentEl = row.querySelector(".tme-seq-content");
		if (contentEl.getAttribute("data-loaded") === "1") return; // already fetched

		fetchFclFile(TRIGGER_SEQUENCES_FCL_PATH, function(fclText, errStr) {
			if (errStr) {
				contentEl.textContent = errStr;
				return;
			}
			var modules = parseArrayBlock(fclText, pathName);
			if (!modules) {
				contentEl.textContent = "Trigger path '" + pathName +
					"' not found in trigSequences.fcl.";
				return;
			}
			contentEl.setAttribute("data-loaded", "1");
			contentEl.innerHTML =
				'<div class="tme-seq-source-line">' + escapeHtml(TRIGGER_SEQUENCES_FCL_PATH) + '</div>' +
				renderModuleList(modules, rowId + "-0");
		});
	}; //end toggleSequence()

	//=====================================================================================
	// Renders a <ol> of sequence items, each individually expandable via
	// TriggerMenuEditor.toggleModule(). Each item's source fcl file is
	// filled in, right-aligned on the item's own line, by
	// finishToggleModule() once that item is resolved.
	function renderModuleList(items, idPrefix) {
		return '<ol class="tme-seq-list">' +
			items.map(function(item, idx) {
				var itemId = idPrefix + "-" + idx;
				return '<li>' +
					'<a onclick="TriggerMenuEditor.toggleModule(\'' + escapeAttr(item) +
					'\',\'' + itemId + '\')">' + escapeHtml(item) + '</a>' +
					'<span id="' + itemId + '-src" class="tme-seq-source-inline"></span>' +
					'<div id="' + itemId + '" class="tme-seq-content tme-seq-nested" style="display:none;"></div>' +
					'</li>';
			}).join('') +
			'</ol>';
	} //end renderModuleList()

	//=====================================================================================
	// Click handler for an individual sequence item: a plain module name
	// (e.g. "CaloEventFilter", looked up in the filters/producers fcl files)
	// or an "@sequence::Prolog.field" reference (looked up in
	// trigRecoSequences.fcl, expanding to another item list).
	TriggerMenuEditor.toggleModule = function(itemName, itemId) {
		var el = document.getElementById(itemId);
		if (!el) return;

		var srcEl = document.getElementById(itemId + "-src");

		if (el.style.display !== "none") {
			el.style.display = "none";
			if (srcEl) srcEl.style.display = "none";
			return;
		}
		el.style.display = "block";
		if (srcEl && srcEl.textContent) srcEl.style.display = "";

		if (el.getAttribute("data-loaded") === "1") return; // already fetched
		el.textContent = "Loading...";

		var sequenceMatch = /^@sequence::([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*)$/.exec(itemName);
		if (sequenceMatch) {
			resolveSequenceReference(sequenceMatch[1], sequenceMatch[2], function(result, errStr, sourceFile) {
				finishToggleModule(el, itemId, result, errStr, sourceFile);
			});
		}
		else {
			resolveModuleBlock(itemName, function(result, errStr, sourceFile) {
				finishToggleModule(el, itemId, result, errStr, sourceFile);
			});
		}
	}; //end toggleModule()

	//=====================================================================================
	function finishToggleModule(el, itemId, result, errStr, sourceFile) {
		if (errStr) {
			el.textContent = errStr;
			return;
		}
		el.setAttribute("data-loaded", "1");

		var srcEl = document.getElementById(itemId + "-src");
		if (srcEl && sourceFile)
			srcEl.textContent = sourceFile;

		if (Array.isArray(result))
			el.innerHTML = renderModuleList(result, itemId);
		else
			el.innerHTML = '<pre class="tme-seq-fcl">' + escapeHtml(result) + '</pre>';
	} //end finishToggleModule()

	//=====================================================================================
	// Resolves "@sequence::prologName.fieldName" by fetching
	// trigRecoSequences.fcl, finding "prologName: { ... fieldName: [ ... ] ... }".
	function resolveSequenceReference(prologName, fieldName, callback) {
		fetchFclFile(TRIGGER_RECO_SEQ_FCL_PATH, function(fclText, errStr) {
			if (errStr) {
				callback(null, errStr);
				return;
			}

			var prologBlock = parseBraceBlock(fclText, prologName);
			if (prologBlock === null) {
				callback(null, "'" + prologName + "' not found in trigRecoSequences.fcl.");
				return;
			}

			var items = parseArrayBlock(prologBlock, fieldName);
			if (!items) {
				callback(null, "'" + fieldName + "' not found in " + prologName + ".");
				return;
			}

			callback(items, null, TRIGGER_RECO_SEQ_FCL_PATH);
		});
	} //end resolveSequenceReference()

	//=====================================================================================
	// Resolves a plain module name (e.g. "CaloEventFilter") by searching all
	// mu2e-trig-config filters/producers fcl files (indexed dynamically from
	// trigFilters.fcl/trigProducers.fcl's #include lines) for a
	// "moduleName: { ... }" block, returning its raw fcl text.
	function resolveModuleBlock(moduleName, callback) {
		fetchFilterProducerFiles(function(paths, errStr) {
			if (errStr) {
				callback(null, errStr);
				return;
			}

			fetchAllFcl(paths, function(contents, fetchErrStr) {
				if (fetchErrStr) {
					callback(null, fetchErrStr);
					return;
				}

				for (var i = 0; i < paths.length; i++) {
					var block = parseBraceBlock(contents[i], moduleName);
					if (block !== null)
					{
						callback(moduleName + ": {" + block + "}", null, paths[i]);
						return;
					}
				}

				callback(null, "Module '" + moduleName +
					"' not found in mu2e-trig-config filters/producers.");
			});
		});
	} //end resolveModuleBlock()

	//=====================================================================================
	// Fetches trigFilters.fcl and trigProducers.fcl, extracts their
	// "#include mu2e-trig-config/..." lines, and returns that path list
	// (cached after first call) -- so the index stays correct even if
	// mu2e-trig-config adds/removes a filter or producer file.
	function fetchFilterProducerFiles(callback) {
		if (_filterProducerFilesPromise) {
			_filterProducerFilesPromise(callback);
			return;
		}

		var resultPaths = null;
		var resultErr   = null;
		var pending     = [];

		_filterProducerFilesPromise = function(cb) {
			if (resultPaths || resultErr) {
				cb(resultPaths, resultErr);
				return;
			}
			pending.push(cb);
		};
		_filterProducerFilesPromise(callback);

		fetchAllFcl([TRIGGER_FILTERS_INDEX_PATH, TRIGGER_PRODUCERS_INDEX_PATH],
			function(contents, errStr) {
				if (errStr) {
					resultErr = errStr;
				}
				else {
					var paths = [];
					for (var i = 0; i < contents.length; i++) {
						var m;
						MU2E_TRIG_CONFIG_INCLUDE_RE.lastIndex = 0;
						while ((m = MU2E_TRIG_CONFIG_INCLUDE_RE.exec(contents[i])) !== null)
							paths.push(m[1]);
					}
					resultPaths = paths;
				}

				var callbacks = pending;
				pending = [];
				callbacks.forEach(function(cb) { cb(resultPaths, resultErr); });
			}
		);
	} //end fetchFilterProducerFiles()

	//=====================================================================================
	// Fetches multiple fcl files (via fetchFclFile, so each is individually
	// cached) and calls back once with an array of contents in the same
	// order, or an error string if any failed.
	function fetchAllFcl(paths, callback) {
		var contents = new Array(paths.length);
		var remaining = paths.length;
		var failed = false;

		if (remaining === 0) {
			callback(contents, null);
			return;
		}

		paths.forEach(function(path, idx) {
			fetchFclFile(path, function(content, errStr) {
				if (failed) return;
				if (errStr) {
					failed = true;
					callback(null, errStr);
					return;
				}
				contents[idx] = content;
				remaining--;
				if (remaining === 0)
					callback(contents, null);
			});
		});
	} //end fetchAllFcl()

	//=====================================================================================
	// Fetches a single fcl file's content by $FHICL_FILE_PATH-relative path,
	// via the Code Editor supervisor's getFhiclFileContent, caching the
	// result so repeated lookups (e.g. multiple modules in the same file)
	// don't re-fetch.
	function fetchFclFile(relativePath, callback) {
		if (_fclFileCache[relativePath] !== undefined) {
			callback(_fclFileCache[relativePath], null);
			return;
		}

		function withUrn(urn) {
			DesktopContent.XMLHttpRequest(
				"Request?RequestType=codeEditor" +
				"&option=getFhiclFileContent" +
				"&path=" + encodeURIComponent(relativePath),
				"",
				function(req) {
					if (!req) {
						callback(null, "Error fetching " + relativePath);
						return;
					}
					var errStr = DesktopContent.getXMLValue(req, "Error");
					if (errStr) {
						callback(null, errStr);
						return;
					}

					var content = DesktopContent.getXMLValue(req, "content");
					_fclFileCache[relativePath] = content;
					callback(content, null);
				},
				undefined, undefined, true,
				true, undefined, undefined, undefined,
				urn
			);
		} //end withUrn()

		if (_codeEditorUrn) {
			withUrn(_codeEditorUrn);
			return;
		}

		DesktopContent.XMLHttpRequest(
			"Request?RequestType=getAppUrnByClass" +
			"&className=" + encodeURIComponent(CODE_EDITOR_SUPERVISOR_CLASS),
			"",
			function(req) {
				if (!req) {
					callback(null, "Error looking up Code Editor application");
					return;
				}
				var errStr = DesktopContent.getXMLValue(req, "Error");
				if (errStr) {
					callback(null, errStr);
					return;
				}

				var urn = DesktopContent.getXMLValue(req, "urn");
				if (!urn) {
					callback(null, "Code Editor application URN not found.");
					return;
				}

				_codeEditorUrn = urn;
				withUrn(urn);
			},
			undefined, undefined, true,
			true
		);
	} //end fetchFclFile()

	//=====================================================================================
	// Parses a "<name>: [ item1, item2, ... ]" fhicl array out of raw fcl
	// text. Returns null if name is not found as an array declaration.
	function parseArrayBlock(fclText, name) {
		return parseDelimitedBlock(fclText, name, '[', ']', function(blockContent) {
			return blockContent.split(",")
				.map(function(s) { return s.trim(); })
				.filter(function(s) { return s.length > 0; });
		});
	} //end parseArrayBlock()

	//=====================================================================================
	// Parses a "<name>: { ... }" fhicl table block out of raw fcl text,
	// returning its raw inner content (not comma-split, since table bodies
	// are fhicl key:value pairs, not simple lists). Returns null if not found.
	function parseBraceBlock(fclText, name) {
		return parseDelimitedBlock(fclText, name, '{', '}', function(blockContent) {
			return blockContent;
		});
	} //end parseBraceBlock()

	//=====================================================================================
	// Shared "find <name> : <open> ... <close>" scanner used by
	// parseArrayBlock/parseBraceBlock. transform(blockContent) shapes the
	// captured inner text into the return value.
	function parseDelimitedBlock(fclText, name, openChar, closeChar, transform) {
		var nameRegex = new RegExp("(^|[^A-Za-z0-9_])" + name.replace(/[.*+?^${}()|[\]\\]/g, "\\$&") +
			"([^A-Za-z0-9_]|$)");

		var searchFrom = 0;
		while (true) {
			var m = nameRegex.exec(fclText.slice(searchFrom));
			if (!m) return null;

			var matchStart = searchFrom + m.index + m[1].length;
			var afterName = matchStart + name.length;

			var p = afterName;
			while (p < fclText.length && /\s/.test(fclText[p])) p++;
			if (fclText[p] === ':') {
				p++;
				while (p < fclText.length && /\s/.test(fclText[p])) p++;
				if (fclText[p] === openChar) {
					var blockStart = p + 1;
					var depth = 1;
					var q = blockStart;
					while (q < fclText.length && depth > 0) {
						if (fclText[q] === openChar) depth++;
						else if (fclText[q] === closeChar) depth--;
						q++;
					}
					if (depth !== 0) return null;

					return transform(fclText.slice(blockStart, q - 1));
				}
			}

			searchFrom = afterName;
		}
	} //end parseDelimitedBlock()

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
