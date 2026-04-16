#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>
#include <stack>

wchar_t* projectPath;

// ==========================================
// GLOBAL STATE VARIABLES (Phase 1 & 2)
// ==========================================
Mat original_img;
Rect current_roi;
std::stack<Rect> roi_history;

// 4-Quadrant Fixed Dimensions (Assuming 1920x1080 monitor)
const int QUAD_W = 800;
const int QUAD_H = 440;

// Window Names
const char* WIN_SRC = "Source Image & Minimap";
const char* WIN_NN = "Nearest Neighbor";
const char* WIN_BILINEAR = "Bilinear Interpolation";
const char* WIN_BICUBIC = "Bicubic Interpolation";

// ==========================================
// PHASE 3 STUBS (The Rendering Engine)
// ==========================================

void computeNearestNeighbor() {
    // Safety check
    if (original_img.empty() || current_roi.width <= 0 || current_roi.height <= 0) return;

    // 1. Create the blank display buffer (QUAD_H x QUAD_W, 8 bits/pixel grayscale)
    Mat dst(QUAD_H, QUAD_W, CV_8UC1);

    // 2. Calculate scaling factors (Source ROI / Destination Window)
    double scale_x = (double)current_roi.width / QUAD_W;
    double scale_y = (double)current_roi.height / QUAD_H;

    // 3. Loop through every pixel in our new display window
    for (int r = 0; r < QUAD_H; r++) {
        for (int c = 0; c < QUAD_W; c++) {

            // 4. Reverse mapping: find the corresponding pixel in the original image ROI
            // Adding 0.5 before casting to int handles the "round to nearest" math
            int src_x = current_roi.x + (int)(c * scale_x + 0.5);
            int src_y = current_roi.y + (int)(r * scale_y + 0.5);

            // 5. Boundary Clamping (Safety check)
            // Ensures floating point math doesn't push us outside the image matrix
            if (src_x < 0) src_x = 0;
            if (src_x >= original_img.cols) src_x = original_img.cols - 1;
            if (src_y < 0) src_y = 0;
            if (src_y >= original_img.rows) src_y = original_img.rows - 1;

            // 6. Copy the 8-bit grayscale pixel value
            dst.at<uchar>(r, c) = original_img.at<uchar>(src_y, src_x);
        }
    }

    // 7. Update the Nearest Neighbor window with actual image data!
    imshow(WIN_NN, dst);
}

void computeBilinear() {
    // Safety check
    if (original_img.empty() || current_roi.width <= 0 || current_roi.height <= 0) return;

    // 1. Create the blank display buffer (QUAD_H x QUAD_W, 8 bits/pixel grayscale)
    Mat dst(QUAD_H, QUAD_W, CV_8UC1);

    // 2. Calculate scaling factors
    double scale_x = (double)current_roi.width / QUAD_W;
    double scale_y = (double)current_roi.height / QUAD_H;

    // 3. Loop through every pixel in our new display window
    for (int r = 0; r < QUAD_H; r++) {
        for (int c = 0; c < QUAD_W; c++) {

            // Step 1: Exact floating point coordinates in the original image
            double exact_x = current_roi.x + (c * scale_x);
            double exact_y = current_roi.y + (r * scale_y);

            // Step 2: Find the 2x2 bounding box coordinates (Floor values)
            int x1 = (int)exact_x;
            int y1 = (int)exact_y;

            // Step 3: Find the ceiling values and clamp them to prevent out-of-bounds crashes
            int x2 = x1 + 1;
            int y2 = y1 + 1;

            // Safety clamps
            if (x1 < 0) x1 = 0;
            if (y1 < 0) y1 = 0;
            if (x2 >= original_img.cols) x2 = original_img.cols - 1;
            if (y2 >= original_img.rows) y2 = original_img.rows - 1;

            // Step 4: Calculate the fractional distances (weights)
            double dx = exact_x - x1;
            double dy = exact_y - y1;

            // Step 5: Grab the 8-bit grayscale intensities of the 4 surrounding pixels
            uchar p11 = original_img.at<uchar>(y1, x1); // Top-Left
            uchar p21 = original_img.at<uchar>(y1, x2); // Top-Right
            uchar p12 = original_img.at<uchar>(y2, x1); // Bottom-Left
            uchar p22 = original_img.at<uchar>(y2, x2); // Bottom-Right

            // Step 6: Apply the Bilinear interpolation formulas
            double top_interp = p11 * (1.0 - dx) + p21 * dx;
            double bottom_interp = p12 * (1.0 - dx) + p22 * dx;
            double final_interp = top_interp * (1.0 - dy) + bottom_interp * dy;

            // Step 7: Assign the blended value (adding 0.5 to round to the nearest integer)
            dst.at<uchar>(r, c) = (uchar)(final_interp + 0.5);
        }
    }

    // 8. Update the Bilinear window with actual image data!
    imshow(WIN_BILINEAR, dst);
}

#include <cmath> // Required for std::abs

// 1. The Mathematical Weight Helper
double cubicWeight(double x) {
    double a = -0.5;
    double abs_x = std::abs(x);

    if (abs_x <= 1.0) {
        return (a + 2.0) * abs_x * abs_x * abs_x - (a + 3.0) * abs_x * abs_x + 1.0;
    }
    else if (abs_x < 2.0) {
        return a * abs_x * abs_x * abs_x - 5.0 * a * abs_x * abs_x + 8.0 * a * abs_x - 4.0 * a;
    }
    return 0.0;
}

// 2. The Bicubic Engine
void computeBicubic() {
    // Safety check
    if (original_img.empty() || current_roi.width <= 0 || current_roi.height <= 0) return;

    // Create the blank display buffer
    Mat dst(QUAD_H, QUAD_W, CV_8UC1);

    // Calculate scaling factors
    double scale_x = (double)current_roi.width / QUAD_W;
    double scale_y = (double)current_roi.height / QUAD_H;

    // Loop through every pixel in our new display window
    for (int r = 0; r < QUAD_H; r++) {
        for (int c = 0; c < QUAD_W; c++) {

            // Exact floating point coordinates in the original image
            double exact_x = current_roi.x + (c * scale_x);
            double exact_y = current_roi.y + (r * scale_y);

            // The integer base coordinate
            int ix = (int)exact_x;
            int iy = (int)exact_y;

            // The fractional distances
            double dx = exact_x - ix;
            double dy = exact_y - iy;

            double pixel_sum = 0.0;

            // The 4x4 Grid Loop (Offsets -1, 0, 1, 2)
            for (int m = -1; m <= 2; m++) {
                for (int n = -1; n <= 2; n++) {

                    // Apply offsets to find the neighbor pixel
                    int cx = ix + n;
                    int cy = iy + m;

                    // EXTREME BOUNDARY CLAMPING
                    // We must protect all 4 sides of the image matrix
                    if (cx < 0) cx = 0;
                    if (cx >= original_img.cols) cx = original_img.cols - 1;
                    if (cy < 0) cy = 0;
                    if (cy >= original_img.rows) cy = original_img.rows - 1;

                    // Calculate weights using our cubic function
                    // We pass in the distance from our exact point to this specific neighbor
                    double weight_x = cubicWeight(n - dx);
                    double weight_y = cubicWeight(m - dy);

                    // Accumulate the weighted pixel value
                    pixel_sum += original_img.at<uchar>(cy, cx) * weight_x * weight_y;
                }
            }

            // Clamp the final sum to the valid 8-bit grayscale range [0, 255]
            if (pixel_sum < 0) pixel_sum = 0;
            if (pixel_sum > 255) pixel_sum = 255;

            // Assign the final calculated color
            dst.at<uchar>(r, c) = (uchar)pixel_sum;
        }
    }

    // Update the Bicubic window with actual image data!
    imshow(WIN_BICUBIC, dst);
}

void updateDisplays() {
    if (original_img.empty()) return;

    // 1. Draw the Minimap on the Source Window
    Mat minimap;
    // We temporarily convert our grayscale clone to BGR just so we can draw a RED rectangle
    cvtColor(original_img, minimap, COLOR_GRAY2BGR);
    rectangle(minimap, current_roi, Scalar(0, 0, 255), 2); // 2 is the line thickness
    imshow(WIN_SRC, minimap);

    // 2. Trigger the Interpolation Engines (Currently just stubs)
    printf("\nRendering Algorithms for ROI -> X:%d Y:%d W:%d H:%d\n",
        current_roi.x, current_roi.y, current_roi.width, current_roi.height);

    computeNearestNeighbor();
    computeBilinear();
    computeBicubic();
}

// ==========================================
// PHASE 2 IMPLEMENTATION (Mouse Math)
// ==========================================

void MyCallBackFunc(int event, int x, int y, int flags, void* param) {
    if (event == EVENT_LBUTTONDOWN) {
        // 1. Map screen click to actual image coordinates based on current ROI
        int orig_x = current_roi.x + (x * current_roi.width / QUAD_W);
        int orig_y = current_roi.y + (y * current_roi.height / QUAD_H);

        // 2. Save state for shrinking later
        roi_history.push(current_roi);

        // 3. Calculate 2x Zoom dimensions
        int new_w = current_roi.width / 2;
        int new_h = current_roi.height / 2;

        // Prevent math errors if we zoom in too far (width/height hitting 0)
        if (new_w < 1) new_w = 1;
        if (new_h < 1) new_h = 1;

        // 4. Center the new ROI on the click
        int new_x = orig_x - (new_w / 2);
        int new_y = orig_y - (new_h / 2);

        // 5. Clamp boundaries so we don't read out of bounds
        if (new_x < 0) new_x = 0;
        if (new_y < 0) new_y = 0;
        if (new_x + new_w > original_img.cols) new_x = original_img.cols - new_w;
        if (new_y + new_h > original_img.rows) new_y = original_img.rows - new_h;

        // 6. Update global state and render
        current_roi = Rect(new_x, new_y, new_w, new_h);
        updateDisplays();
    }
    else if (event == EVENT_RBUTTONDOWN) {
        // Pop the stack to Zoom Out / Shrink
        if (!roi_history.empty()) {
            current_roi = roi_history.top();
            roi_history.pop();
            updateDisplays();
        }
        else {
            printf("Already at 1x scale (maximum shrink)!\n");
        }
    }
}

// ==========================================
// PHASE 1 IMPLEMENTATION (UI & Loading)
// ==========================================

void testInteractiveZoom()
{
    char fname[MAX_PATH];
    while (openFileDlg(fname))
    {
        // Force 8-bit grayscale loading
        original_img = imread(fname, IMREAD_GRAYSCALE);
        if (original_img.empty()) {
            printf("Could not open image.\n");
            continue;
        }

        // Initialize state
        current_roi = Rect(0, 0, original_img.cols, original_img.rows);
        while (!roi_history.empty()) roi_history.pop();

        // Spawn Windows
        namedWindow(WIN_SRC, WINDOW_AUTOSIZE); // 1:1 Size
        namedWindow(WIN_NN, WINDOW_NORMAL);
        namedWindow(WIN_BILINEAR, WINDOW_NORMAL);
        namedWindow(WIN_BICUBIC, WINDOW_NORMAL);

        // Resize Algorithm Windows to fit the quadrants
        resizeWindow(WIN_NN, QUAD_W, QUAD_H);
        resizeWindow(WIN_BILINEAR, QUAD_W, QUAD_H);
        resizeWindow(WIN_BICUBIC, QUAD_W, QUAD_H);

        // Position into 4 Quadrants (+30 Y-offset to account for OS title bars)
        moveWindow(WIN_SRC, 0, 0);
        moveWindow(WIN_NN, QUAD_W, 0);
        moveWindow(WIN_BILINEAR, 0, QUAD_H + 30);
        moveWindow(WIN_BICUBIC, QUAD_W, QUAD_H + 30);

        // Attach Mouse Listener ONLY to the 3 Algorithm Windows
        setMouseCallback(WIN_NN, MyCallBackFunc, NULL);
        setMouseCallback(WIN_BILINEAR, MyCallBackFunc, NULL);
        setMouseCallback(WIN_BICUBIC, MyCallBackFunc, NULL);

        // Initial render
        updateDisplays();

        printf("Image loaded! Left-Click on any algorithm window to Zoom In. Right-Click to Zoom Out. Press ESC to close.\n");

        while (waitKey(0) != 27) {}
        destroyAllWindows();
    }
}

int main()
{
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);
    projectPath = _wgetcwd(0, 0);
    int op;
    do
    {
        system("cls");
        destroyAllWindows();
        printf("==========================================\n");
        printf(" Project 5: Zooming and Shrinking\n");
        printf("==========================================\n");
        printf(" 1 - Open 4-Quadrant Image Viewer\n");
        printf(" 0 - Exit\n");
        printf("Option: ");
        scanf("%d", &op);

        switch (op)
        {
        case 1:
            testInteractiveZoom();
            break;
        }
    } while (op != 0);
    return 0;
}