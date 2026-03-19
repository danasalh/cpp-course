#include <iostream>
#include <opencv2/opencv.hpp>
 using namespace std; 
 using namespace cv;

 
int main() {
    VideoCapture cam(0);

    if (!cam.isOpened()) {
        cerr << "Error: Could not open the camera.\n";
        return 1;
    }

    cout << "Camera opened successfully! Press 'q' to quit.\n";

    Mat frame;

    while (true) {
        cam >> frame;

        if (frame.empty()) {
            cerr << "Error: Blank frame grabbed.\n";
            break;
        }

        imshow("Camera Test", frame);

        if (waitKey(1) == 'q') {
            break;
        }
    }

    cam.release();
    destroyAllWindows();
    return 0;
}
