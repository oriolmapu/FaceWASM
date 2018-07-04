var Module = {};
importScripts('FaceWASM.js', 'FaceWrapper.js');
Module['onRuntimeInitialized'] = function() { postMessage({msg: 'init'});};

		
		



