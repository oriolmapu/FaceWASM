var init;
var ptr;
var cvbridge;

function initMemory(imageData) {

	cvbridge = new Module.CVBridge(imageData.width,imageData.height);
	ptr      = Module._malloc(imageData.width * imageData.height * 4);

}

function processFrame(imageData) {

	let facialPointsArray = [];

	if (ptr === undefined) {
		initMemory(imageData);
	}

  Module.HEAPU8.set(imageData.data,ptr);
	let facialPoints = cvbridge.processFrame(ptr);
	let YPR = [];
	
	for (let i=0; i<3; i++) {
		YPR.push(facialPoints[i]*Math.PI/180);
	}
	
  for (let i=0; i < facialPoints[3].size(); i++) {
    facialPointsArray.push({
      x: facialPoints[3].get(i)[0],
      y: facialPoints[3].get(i)[1]
    });
	}
	
	let scale = facialPoints[4];

	postMessage({ features: facialPointsArray, YPR: YPR, scale: scale });

}

self.onmessage = function (e) {
	switch (e.data.cmd) {
		case 'faceDetect':
			processFrame(e.data.img);
			break;
		case 'exit':
			if (cvbridge !== undefined)
				cvbridge.delete();
		    if (ptr !== undefined)
		    	Module._free(ptr);	
		    break;
	}
}

self.onerror = function (e) {
	console.log(e);
}


