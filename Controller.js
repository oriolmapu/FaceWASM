let ws   = {};
let faceworker = new Worker('FaceWorker.js'); 

function setWebcam () {

	var constraints = { audio: false, video: {width:640, height:480}, video: { facingMode: "user" } };
	
	navigator.mediaDevices.getUserMedia(constraints).then(function(stream) {
		 ws.video.srcObject = stream;
		 ws.video.onloadedmetadata = function(e) {
			ws.video.play();
		};
	 })
	 .catch(function(err) {
		 console.log(err.name);
	});

}

function setWorkspace() {

	ws.video = document.querySelector('#videoElement'); 
	
	ws.width  = 640;
	ws.height = 480;
	ws.scale  = 1;

	ws.dummy = {};
	ws.dummy.canvas = document.getElementById('dummy');
	ws.dummy.context = ws.dummy.canvas.getContext('2d');
	ws.dummy.canvas.width = ws.width / ws.scale;
	ws.dummy.canvas.height = ws.height / ws.scale;
	
	ws.dummygl = {};
	ws.dummygl.canvas = document.getElementById('dummygl');
	ws.dummygl.context = ws.dummygl.canvas.getContext('2d');
	ws.dummygl.canvas.width = ws.width;
	ws.dummygl.canvas.height = ws.height;
	
	ws.viewers = document.getElementById('viewers');
	
	ws.viewer = {};
	ws.viewer.color_G = 'rgba(0, 255, 0, 1)';
	ws.viewer.color_Y = 'rgba(255, 255, 0, 1)';

	ws.viewer.canvas = document.getElementById('viewer');
	ws.viewer.context = ws.viewer.canvas.getContext('2d');
	ws.viewer.canvas.width = ws.width;
	ws.viewer.canvas.height = ws.height;
	
	/////////////////////////////////////////////
	// Define the scene for the WebGL context //
	////////////////////////////////////////////
	ws.gl = {};
	// Create Scene
	ws.gl.scene  = new THREE.Scene();
	ws.gl.aspect = ws.width/ws.height;
	// Create Renderer
	ws.gl.renderer = new THREE.WebGLRenderer( {antialias:true} );
	ws.gl.renderer.setSize(ws.width, ws.height);
	let canvasgl = ws.gl.renderer.domElement;
	canvasgl.style = "max-width: 100%; height: auto;";
	ws.viewers.appendChild(canvasgl);
	// Create camera	
	const VIEW_ANGLE = 45, ASPECT = ws.gl.aspect, NEAR = 0.1, FAR = 1000;
	ws.gl.camera = new THREE.OrthographicCamera( ws.width / - 2, ws.width / 2, ws.height / 2, ws.height / - 2, NEAR, FAR );
	//ws.gl.camera = new THREE.PerspectiveCamera( VIEW_ANGLE, ASPECT, NEAR, FAR);
	// TODO: Avoid camera hard coding
	ws.gl.camera.position.set(0,0,579);
	ws.gl.camera.lookAt(0, 0, 0);
	ws.gl.scene.add(ws.gl.camera);
	// Inject texture
	ws.gl.videoTexture = new THREE.Texture( ws.video );
	ws.gl.videoTexture.minFilter = THREE.LinearFilter;
	ws.gl.videoTexture.magFilter = THREE.LinearFilter;
	ws.gl.geometry = new THREE.PlaneGeometry( ws.width, ws.height, 1, 1 );
	ws.gl.material = new THREE.MeshBasicMaterial( { map: ws.gl.videoTexture, overdraw: true } );
	ws.gl.screen   = new THREE.Mesh( ws.gl.geometry, ws.gl.material );
	ws.gl.screen.position.set(0,0,0);
	ws.gl.ratioPixels = {};
	ws.gl.ratioPixels.x = ws.width / ws.gl.screen.geometry.parameters.width;
	ws.gl.ratioPixels.y = ws.height / ws.gl.screen.geometry.parameters.height;
	ws.gl.scene.add(ws.gl.screen);
	// Landmarks
	ws.gl.landmarks = {};
	ws.gl.landmarks.geometry = new THREE.BufferGeometry();
	ws.gl.landmarks.positions = new Float32Array( 68 * 3 );
	ws.gl.landmarks.geometry.addAttribute( 'position', new THREE.BufferAttribute( ws.gl.landmarks.positions, 3 ) );
	ws.gl.landmarks.material = new THREE.PointsMaterial( { color: 0x00ff00, size: 5 } );
  	ws.gl.landmarks.points = new THREE.Points(ws.gl.landmarks.geometry,ws.gl.landmarks.material);
  	ws.gl.landmarks.points.position.set(0,0,0);
	ws.gl.scene.add(ws.gl.landmarks.points);	
	// Sun Glasses
	ws.gl.glasses = {};
	ws.gl.glasses.mtlLoader = new THREE.MTLLoader();
	ws.gl.glasses.mtlLoader.load('objects/RayBanz.mtl', function(materials) {
	  materials.preload();
	  ws.gl.glasses.objLoader = new THREE.OBJLoader();
	  ws.gl.glasses.objLoader.setMaterials(materials);
	  ws.gl.glasses.objLoader.load('objects/RayBanz.obj', function(object) {
		ws.gl.glasses.object = object;
		ws.gl.glasses.object.position.set(0,0,0);
		ws.gl.glasses.object.scale.set(200/640,200/640,200/640);
		ws.gl.glasses.object.visible = false;
		ws.gl.scene.add(object);
	  }, null, null);
	});

	// Create light
	ws.gl.light = new THREE.AmbientLight( 0x404040 );
	ws.gl.scene.add( ws.gl.light );
	 
}

function init() {
	processFrame('faceDetect');
}

function processFrame(command) {
	
	ws.dummy.context.drawImage(ws.video, 0, 0, ws.viewer.canvas.width, ws.viewer.canvas.height, 
										 0, 0, ws.dummy.canvas.width, ws.dummy.canvas.height);
    let message = {
        cmd: command,
        img: ws.dummy.context.getImageData(0, 0, ws.dummy.canvas.width, ws.dummy.canvas.height)
	};
	
    faceWorker.postMessage(message);
}

function updateCanvas2D(e) {
    
	if ( ws.video.readyState === ws.video.HAVE_ENOUGH_DATA ) {
		
		ws.viewer.context.drawImage(ws.video, 0, 0, ws.viewer.canvas.width, ws.viewer.canvas.height);
		ws.viewer.context.lineWidth = 1;
	
		ws.viewer.context.strokeStyle = ws.viewer.color_Y;
		ws.viewer.context.beginPath();
	
		for (let i=0; i <  e.data.features.length-52; i++) {
			let current = e.data.features[i];
			let next    = e.data.features[i+1]
			ws.viewer.context.moveTo((current.x)*ws.scale,(current.y)*ws.scale);
			ws.viewer.context.lineTo((next.x)*ws.scale,(next.y)*ws.scale);
		}
	
		ws.viewer.context.closePath();
		ws.viewer.context.stroke();
	
		ws.viewer.context.strokeStyle = ws.viewer.color_G;
	
	    for (let i=17; i< e.data.features.length; i++) {
	    	coord = e.data.features[i];
		  	ws.viewer.context.strokeRect((coord.x)*ws.scale,(coord.y)*ws.scale,1,1);
		}
  
	}
    
}

function rotateObject(obj,anglex,angley,anglez) {

	anglex = anglex - obj.rotation.x;
	angley = angley - obj.rotation.y;
	anglez = anglez - obj.rotation.z;
	obj.rotateX(anglex);
	obj.rotateY(angley);
	obj.rotateZ(anglez);

}

function updateCanvas3D(e) {
	
	if ( ws.video.readyState === ws.video.HAVE_ENOUGH_DATA ) {
		
		ws.dummygl.context.drawImage(ws.video, 0, 0, ws.viewer.canvas.width, ws.viewer.canvas.height);
		ws.gl.videoTexture.needsUpdate = true;
		
		let landmarks = ws.gl.landmarks.points.geometry.attributes.position.array;
		let index = 0;
		let zaxis = 1; 
		
		let YPR   = e.data.YPR;
		let scale = e.data.scale;
		
		for (let i=0; i< e.data.features.length; i++) {
			let coord = e.data.features[i];
			landmarks[ index ++ ] =  ((coord.x * ws.scale)-ws.width/2)   / ws.gl.ratioPixels.x;
			landmarks[ index ++ ] = -((coord.y * ws.scale)-ws.height/2) /  ws.gl.ratioPixels.y;
			landmarks[ index ++ ] = zaxis;
		}

		if (e.data.features.length == 0) {
			ws.gl.landmarks.points.visible = false;
			ws.gl.glasses.object.visible = false;
		}
		else { 
			// Build the appropiate theoretical model
			ws.gl.landmarks.points.visible = true;
			rotateObject(ws.gl.glasses.object,YPR[1],-YPR[0],-YPR[2]+3*Math.PI/180)
			ws.gl.glasses.object.scale.set(scale*128/640,scale*128/640,scale*128/640);
			//ws.gl.glasses.object.position.set(landmarks[33*3],landmarks[33*3+1],-50);
			ws.gl.glasses.object.position.set(landmarks[33*3]+(100*Math.sin(YPR[0])),landmarks[33*3+1]+(100*Math.sin(YPR[1]))+10,-20);
			ws.gl.glasses.object.visible = true;
		}
		
		ws.gl.landmarks.points.geometry.attributes.position.needsUpdate = true;
		ws.gl.renderer.render( ws.gl.scene, ws.gl.camera );
	
	}
}

faceWorker.onmessage = function (e) {
    if (e.data.msg == 'init') {
   		init();
    }
    else {
    	requestAnimationFrame((initTime) => {
    		updateCanvas2D(e);
            updateCanvas3D(e);
            processFrame('faceDetect')
        });
    }
}


window.onerror = function (event) {
    console.log(event)
};

window.onbeforeunload = function() {
	let message = {
			cmd: 'exit'
	};
	faceWorker.postMessage(message);
}

setWorkspace();
setWebcam();

