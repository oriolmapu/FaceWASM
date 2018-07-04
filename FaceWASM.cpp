
#include "Face.h"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace std;
using namespace cv;
using namespace emscripten;

struct jsPt {
    float x;
    float y;
};

struct jsObj {
	float R;
	float P;
	float Y;
	vector<jsPt> vr;
	float S;
};

class CVBridge {

	public:
	
		bool tracker;
		float valid,tolerance;
		int patchsz, padding;
		Face* t;
		int width,height;
		Mat frame, grayframe;
		Mat patch;
		Mat world2patch;
		Mat patch2world;
		vector<Rect> faces;
		std::vector<cv::Point2f> wPts;
		std::vector<cv::Point2f> esPts;
		Mat esPtsVec;
		Mat wPtsVec;


		CVBridge(int width, int height) {

			/********************/
			/* INITIALITZATIONS */
			tracker = false;
			valid = 0.0f;
			tolerance = 0.6f;			
			this->width  = width;
			this->height = height;

			// Define the template
			patchsz = 129;
			padding = 100;
			t = new Face(patchsz, padding);

			// Initialize the image
			patch = Mat(t->width, t->height, CV_8UC1, Scalar(0));
			
			// Initialize Mapping transformations
			world2patch = Mat(2, 3, CV_32FC1);
			patch2world = Mat(2, 3, CV_32FC1);

			esPtsVec = Mat(136, 1, CV_32FC1);
			wPtsVec  = Mat(136, 1, CV_32FC1);

		}

		jsObj processFrame(int buffer) {

			jsPt r;
			jsObj obj;

			uchar* bufferptr = reinterpret_cast<uchar*>(buffer);
			frame = Mat(height, width, CV_8UC4, bufferptr);
			
			cvtColor(frame, grayframe, CV_RGBA2GRAY );

			if (!tracker) {
	
				// Detect the face using Viola & Jones
				t->detectFaces(grayframe, this->faces);
				
				for (int f=0; f < faces.size(); f++) {
					esPts = t->meanPts;
					t->ipatch2tmplt(grayframe, patch, world2patch, patch2world, faces[f]);
					valid = t->estimateLandmarks(patch, esPts, t->omegaD);
				
					if (valid > tolerance) {
						// Transform estimated landmarks from tmplt to world
						transform(esPts, wPts, patch2world);
						t->ipatch2tmpltPro(grayframe, patch, world2patch, patch2world, wPts);
						transform(wPts, esPts, world2patch);

						obj.S = sqrt(pow(patch2world.at<float>(0, 0), 2) + pow(patch2world.at<float>(1, 1), 2));

						for (int j = 0; j < (this->wPts).size(); j++) {
							r.x = wPts[j].x;
							r.y = wPts[j].y;
							obj.vr.push_back(r);
						}
						// 3D pose
						for (int j = 0; j < 68; j++) {
							wPtsVec.at<float>(j) = wPts[j].x - wPts[30].x;
							wPtsVec.at<float>(68 + j) = wPts[j].y - wPts[30].y;
						}

						double max, min;
						minMaxLoc(abs(wPtsVec.rowRange(0, 67)), &min, &max, 0, 0);
						wPtsVec = wPtsVec.mul(1 / max);

						Mat nshape;
						cv::vconcat(Mat::ones(1, 1, CV_32FC1), wPtsVec, nshape);
						Mat RPY = t->omega3D[0].t() * nshape;
						obj.R = RPY.at<float>(0);
						obj.P = RPY.at<float>(1);
						obj.Y = RPY.at<float>(2);
						tracker = true;
						cout << "msg:initializing face"  << endl;
						break;
					}
				}
				faces.clear();
			}
			else {

				t->ipatch2tmpltPro(grayframe, patch, world2patch, patch2world, wPts);
				transform(wPts, esPts, world2patch);

				for (int j = 0; j < 68; j++) {
					esPtsVec.at<float>(j) = esPts[j].x;
					esPtsVec.at<float>(68 + j) = esPts[j].y;
				}

				// Project and reconstruct to constrain initialization
				esPtsVec = t->muS[0] + t->eigS[0]  * (t->eigS[0].t()*(esPtsVec - t->muS[0]));

				for (int j = 0; j < 68; j++) {
					esPts[j].x = esPtsVec.at<float>(j);
					esPts[j].y = esPtsVec.at<float>(68 + j);
				}

				valid = t->estimateLandmarks(patch, esPts, t->omegaT);

				if (valid > tolerance) {
					// Transform estimated landmarks from tmplt to world
					transform(esPts, wPts, patch2world);
					t->ipatch2tmpltPro(grayframe, patch, world2patch, patch2world, wPts);
					transform(wPts, esPts, world2patch);

					obj.S = sqrt(pow(patch2world.at<float>(0, 0), 2) + pow(patch2world.at<float>(1, 1), 2));


					for (int j = 0; j < (this->wPts).size(); j++) {
						r.x = wPts[j].x;
						r.y = wPts[j].y;
						obj.vr.push_back(r);
					}
					// 3D pose
					for (int j = 0; j < 68; j++) {
						wPtsVec.at<float>(j) = wPts[j].x - wPts[30].x;
						wPtsVec.at<float>(68 + j) = wPts[j].y - wPts[30].y;
					}

					double max, min;
					minMaxLoc(abs(wPtsVec.rowRange(0, 67)), &min, &max, 0, 0);
					wPtsVec = wPtsVec.mul(1 / max);

					Mat nshape;
					cv::vconcat(Mat::ones(1, 1, CV_32FC1), wPtsVec, nshape);
					Mat RPY = t->omega3D[0].t() * nshape;
					obj.R = RPY.at<float>(0);
					obj.P = RPY.at<float>(1);
					obj.Y = RPY.at<float>(2);

				}
				else {
					// Fallback to detection mode
					tracker = false;
				}
			}


			
			return obj;
		}

};


EMSCRIPTEN_BINDINGS(cvbridge) {

	
	value_array<jsPt>("jsPt")
		.element(&jsPt::x)
		.element(&jsPt::y)
		;
	
	register_vector<jsPt>("vPts");

	value_array<jsObj>("jsObj")
        	.element(&jsObj::R)
        	.element(&jsObj::P)
        	.element(&jsObj::Y)
		    .element(&jsObj::vr)
			.element(&jsObj::S)
        ;

	class_<CVBridge>("CVBridge")
		.constructor<int,int>()
		.function("processFrame",&CVBridge::processFrame)
	;

}

