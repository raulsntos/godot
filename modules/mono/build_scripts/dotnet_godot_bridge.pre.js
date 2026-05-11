/* global _scriptName, UTF8ArrayToString, wasmExports, stackSave, stackRestore, stackAlloc */

// Godot hosts the Mono WebAssembly runtime inside the engine module, instead of
// letting dotnet.js create its own Emscripten module. Load the .NET runtime
// helpers here so dotnet.es6.lib.js can wire the generated import stubs before
// Godot instantiates the wasm.
const __godotDotnetRuntimeUrl = new URL(_scriptName || globalThis.document?.currentScript?.src || globalThis.location?.href);
__godotDotnetRuntimeUrl.pathname = __godotDotnetRuntimeUrl.pathname.replace(/\.js$/, '.dotnet.runtime.js');

const __godotDotnetRuntime = await import(__godotDotnetRuntimeUrl.href);
globalThis.__dotnet_runtime = __godotDotnetRuntime;

if (!Module.config) {
	Module.config = {};
}
if (!Module.ready) {
	Module.ready = new Promise(function () {});
}
Module.safeSetTimeout = function (fn, ms) {
	return setTimeout(fn, ms);
};

const __godotDotnetInternal = {};
const __godotDotnetRuntimeHelpers = {};
const __godotDotnetLoaderHelpers = {
	createPromiseController: function () {
		let resolve;
		let reject;
		const promise = new Promise(function (res, rej) {
			resolve = res;
			reject = rej;
		});
		return {
			promise,
			promise_control: { resolve, reject },
		};
	},
	assert_runtime_running: function () {},
	is_runtime_running: function () {
		return true;
	},
	mono_exit: function (code, err) {
		throw err || new Error(`exit:${code}`);
	},
	fetch_like: globalThis.fetch.bind(globalThis),
	scriptDirectory: new URL('.', __godotDotnetRuntimeUrl).href,
	gitHash: '',
	config: { globalizationMode: 'icu' },
	installUnhandledErrorHandler: function () {},
	logDownloadStatsToConsole: function () {},
	purgeUnusedCacheEntriesAsync: function () {},
};
const __godotDotnetGlobalObjects = {
	module: Module,
	internal: __godotDotnetInternal,
	runtimeHelpers: __godotDotnetRuntimeHelpers,
	loaderHelpers: __godotDotnetLoaderHelpers,
	globalizationHelpers: {},
	api: {},
};

__godotDotnetRuntime.setRuntimeGlobals(__godotDotnetGlobalObjects);
__godotDotnetRuntime.initializeExports(__godotDotnetGlobalObjects);
await __godotDotnetRuntime.configureRuntimeStartup(Module);

function __godotDotnetReplaceStubs(imports) {
	const importModule = imports && (imports.env || imports.a);
	if (!importModule) {
		return;
	}

	for (const key of Object.keys(importModule)) {
		const stub = importModule[key];
		if (typeof stub !== 'function') {
			continue;
		}

		let stubName = stub.name || '';
		if (!stubName || stubName === 'anonymous') {
			const source = Function.prototype.toString.call(stub);
			const match = source.match(/\/\/\s*([A-Za-z0-9_]+)/) || source.match(/function\s+([A-Za-z0-9_$]+)/);
			stubName = match ? match[1] : '';
		}

		const helperName = stubName.replace(/^_+/, '');
		const replacement = __godotDotnetRuntimeHelpers[helperName];
		if (typeof replacement === 'function') {
			importModule[key] = replacement;
		}
	}
}

const __godotDotnetInstantiateStreaming = WebAssembly.instantiateStreaming;
const __godotDotnetInstantiate = WebAssembly.instantiate;
WebAssembly.instantiateStreaming = function (source, imports) {
	__godotDotnetReplaceStubs(imports);
	return __godotDotnetInstantiateStreaming.call(WebAssembly, source, imports);
};
WebAssembly.instantiate = function (source, imports) {
	__godotDotnetReplaceStubs(imports);
	return __godotDotnetInstantiate.call(WebAssembly, source, imports);
};

const __godotDotnetOnRuntimeInitialized = Module.onRuntimeInitialized;
Module.onRuntimeInitialized = function () {
	__godotDotnetLoaderHelpers.is_runtime_running = function () {
		return true;
	};
	__godotDotnetRuntimeHelpers.runtimeReady = true;

	if (typeof UTF8ArrayToString !== 'undefined') {
		Module.UTF8ArrayToString = UTF8ArrayToString;
	}
	if (typeof lengthBytesUTF8 !== 'undefined') {
		Module.lengthBytesUTF8 = lengthBytesUTF8;
	}
	if (typeof stringToUTF8Array !== 'undefined') {
		Module.stringToUTF8Array = stringToUTF8Array;
	}
	if (typeof wasmExports !== 'undefined') {
		Module.wasmExports = wasmExports;
		for (const key of Object.keys(wasmExports)) {
			const helperName = key.replace(/^_+/, '');
			if (helperName.startsWith('mono_wasm_') && !__godotDotnetRuntimeHelpers[helperName]) {
				__godotDotnetRuntimeHelpers[helperName] = wasmExports[key];
			}
		}
	}
	if (typeof _malloc !== 'undefined') {
		Module._malloc = _malloc;
	}
	if (typeof _free !== 'undefined') {
		Module._free = _free;
	}
	if (typeof stackSave !== 'undefined') {
		Module.stackSave = stackSave;
	}
	if (typeof stackRestore !== 'undefined') {
		Module.stackRestore = stackRestore;
	}
	if (typeof stackAlloc !== 'undefined') {
		Module.stackAlloc = stackAlloc;
	}

	if (__godotDotnetOnRuntimeInitialized) {
		__godotDotnetOnRuntimeInitialized();
	}
};

Module.__dotnet_runtime = {
	initializeReplacements: __godotDotnetRuntime.initializeReplacements,
	passEmscriptenInternals: __godotDotnetRuntime.passEmscriptenInternals,
	configureEmscriptenStartup: function () {},
	configureWorkerStartup: function () {},
};
