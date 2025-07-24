#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QStringList>
#include <QDateTime>
#include <QByteArray>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

class TesseractOCR {
public:
    TesseractOCR() {
        api = nullptr;

        // Set the path to tessdata directory
        tessdataPath = "C:/Program Files/Tesseract-OCR/tessdata";

        // Supported image extensions
        supportedExtensions << "*.png" << "*.jpg" << "*.jpeg"
                           << "*.tiff" << "*.tif" << "*.bmp"
                           << "*.gif" << "*.webp";
    }

    ~TesseractOCR() {
        cleanup();
    }

    bool initialize(const QString& language = "eng",int pageSegmentationMode = 6) {
        cleanup(); // Clean up any existing instance

        api = new tesseract::TessBaseAPI();

        // Initialize tesseract-ocr with language
        if (api->Init(tessdataPath.toStdString().c_str(), language.toStdString().c_str())) {
            qDebug() << "Could not initialize tesseract with language:" << language;
            qDebug() << "Make sure tessdata path exists:" << tessdataPath;

            // Check if tessdata path exists
            QDir tessdataDir(tessdataPath);
            if (!tessdataDir.exists()) {
                qDebug() << "ERROR: tessdata directory does not exist!";
                std::cout << "ERROR: tessdata directory does not exist: " << tessdataPath.toStdString() << std::endl;
            }

            delete api;
            api = nullptr;
            return false;
        }
        // Set Page Segmentation Mode
           api->SetPageSegMode(static_cast<tesseract::PageSegMode>(pageSegmentationMode));


        qDebug() << "Tesseract initialized successfully with language:" << language;
        std::cout << "Tesseract initialized successfully with language: " << language.toStdString() << std::endl;
        return true;
    }

    void cleanup() {
        if (api) {
            api->End();
            delete api;
            api = nullptr;
        }
    }


    QString processImage(const QString& imagePath) {
        if (!api) {
            qDebug() << "Tesseract not initialized. Call initialize() first.";
            std::cout << "ERROR: Tesseract not initialized!" << std::endl;
            return QString();
        }

        // Load image using OpenCV
        cv::Mat image = cv::imread(imagePath.toStdString());
        if (image.empty()) {
            qDebug() << "Could not load image:" << imagePath;
            std::cout << "ERROR: Could not load image: " << imagePath.toStdString() << std::endl;
            return QString();
        }

        std::cout << "Successfully loaded image: " << imagePath.toStdString() << std::endl;

        // Convert to grayscale if needed
        if (image.channels() == 3) {
            cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
        }

        // Set image data in Tesseract
        api->SetImage(image.data, image.cols, image.rows, 1, image.step);

        // Get OCR result
        char* outText = api->GetUTF8Text();
        if (!outText) {
            qDebug() << "OCR failed for image:" << imagePath;
            std::cout << "ERROR: OCR failed for image: " << imagePath.toStdString() << std::endl;
            return QString();
        }

        QString result = QString::fromUtf8(outText);
        delete[] outText;

        std::cout << "OCR completed for: " << imagePath.toStdString() << std::endl;
        return result;
    }

    QString processImageWithConfidence(const QString& imagePath, int minConfidence = 60) {
        if (!api) {
            qDebug() << "Tesseract not initialized. Call initialize() first.";
            std::cout << "ERROR: Tesseract not initialized!" << std::endl;
            return QString();
        }

        // Load image using OpenCV
        cv::Mat image = cv::imread(imagePath.toStdString());
        if (image.empty()) {
            qDebug() << "Could not load image:" << imagePath;
            std::cout << "ERROR: Could not load image: " << imagePath.toStdString() << std::endl;
            return QString();
        }

        std::cout << "Successfully loaded image: " << imagePath.toStdString() << std::endl;

        // Convert to grayscale if needed
        if (image.channels() == 3) {
            cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
        }

        // Set image data in Tesseract
        api->SetImage(image.data, image.cols, image.rows, 1, image.step);

        // Get mean confidence
        int confidence = api->MeanTextConf();
        qDebug() << "OCR confidence for" << QFileInfo(imagePath).fileName() << ":" << confidence << "%";
        std::cout << "OCR confidence for " << QFileInfo(imagePath).fileName().toStdString()
                  << ": " << confidence << "%" << std::endl;

        if (confidence < minConfidence) {
            qDebug() << "Low confidence (" << confidence << "%), skipping result for:" << imagePath;
            std::cout << "Low confidence (" << confidence << "%), skipping result for: "
                      << imagePath.toStdString() << std::endl;
            return QString();
        }

        // Get OCR result
        char* outText = api->GetUTF8Text();
        if (!outText) {
            qDebug() << "OCR failed for image:" << imagePath;
            std::cout << "ERROR: OCR failed for image: " << imagePath.toStdString() << std::endl;
            return QString();
        }

        QString result = QString::fromUtf8(outText);
        delete[] outText;

        std::cout << "OCR completed successfully for: " << imagePath.toStdString() << std::endl;
        return result;
    }

    bool saveToFile(const QString& text, const QString& outputPath) {
        QFile file(outputPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Could not create output file:" << outputPath;
            std::cout << "ERROR: Could not create output file: " << outputPath.toStdString() << std::endl;
            return false;
        }

        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << text;
        file.close();

        qDebug() << "Text saved to:" << outputPath;
        std::cout << "Text saved to: " << outputPath.toStdString() << std::endl;
        return true;
    }

    bool processFolder(const QString& folderPath, const QString& outputFile,
                      const QString& language = "rus+ukr", bool useConfidence = false, int minConfidence = 60,int pageSegmentationMode = 6) {

        std::cout << "Starting processFolder with:" << std::endl;
        std::cout << "  Folder: " << folderPath.toStdString() << std::endl;
        std::cout << "  Output: " << outputFile.toStdString() << std::endl;
        std::cout << "  Language: " << language.toStdString() << std::endl;

        if (!initialize(language,pageSegmentationMode)) {
            std::cout << "ERROR: Failed to initialize Tesseract!" << std::endl;
            return false;
        }

        QDir inputDir(folderPath);
        if (!inputDir.exists()) {
            qDebug() << "Input folder does not exist:" << folderPath;
            std::cout << "ERROR: Input folder does not exist: " << folderPath.toStdString() << std::endl;
            return false;
        }

        std::cout << "Input folder exists and is accessible." << std::endl;

        // Get all image files in the folder
        QStringList imageFiles;
        for (const QString& extension : supportedExtensions) {
            QStringList matchingFiles = inputDir.entryList(QStringList() << extension, QDir::Files);
            imageFiles.append(matchingFiles);
            std::cout << "Found " << matchingFiles.count() << " files matching "
                      << extension.toStdString() << std::endl;
        }

        if (imageFiles.isEmpty()) {
            qDebug() << "No image files found in folder:" << folderPath;
            std::cout << "ERROR: No image files found in folder: " << folderPath.toStdString() << std::endl;

            // List all files in directory for debugging
            QStringList allFiles = inputDir.entryList(QDir::Files);
            std::cout << "All files in directory:" << std::endl;
            for (const QString& file : allFiles) {
                std::cout << "  " << file.toStdString() << std::endl;
            }

            return false;
        }

        qDebug() << "Found" << imageFiles.count() << "image files to process";
        std::cout << "Found " << imageFiles.count() << " image files to process" << std::endl;

        // Create/open the single output file
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Could not create output file:" << outputFile;
            std::cout << "ERROR: Could not create output file: " << outputFile.toStdString() << std::endl;
            return false;
        }

        std::cout << "Output file created successfully." << std::endl;

        QTextStream out(&file);
        out.setCodec("UTF-8");

        int successCount = 0;
        int failCount = 0;

        // Write header to the file
        out << "OCR Results for folder: " << folderPath << "\n";
        out << "Generated on: " << QDateTime::currentDateTime().toString() << "\n";
        out << "Language: " << language << "\n";
        out << "Total images: " << imageFiles.count() << "\n";
        if (useConfidence) {
            out << "Minimum confidence: " << minConfidence << "%\n";
        }
        out << QString("=").repeated(80) << "\n\n";

        // Process each image file
        for (const QString& fileName : imageFiles) {
            QString fullImagePath = inputDir.absoluteFilePath(fileName);

            qDebug() << "\n--- Processing:" << fileName << "---";
            std::cout << "\n--- Processing: " << fileName.toStdString() << " ---" << std::endl;

            QString ocrResult;
            if (useConfidence) {
                ocrResult = processImageWithConfidence(fullImagePath, minConfidence);
            } else {
                ocrResult = processImage(fullImagePath);
            }

            if (!ocrResult.isEmpty()) {
                // Write to single file with filename header
                out << "File: " << fileName << "\n";
                out << QString("-").repeated(40) << "\n";
                out << ocrResult.trimmed() << "\n\n";
                out << QString("=").repeated(80) << "\n\n";

                qDebug() << "✓ Successfully processed:" << fileName;
                std::cout << "✓ Successfully processed: " << fileName.toStdString() << std::endl;
                successCount++;
            } else {
                // Write failure notice to file
                out << "File: " << fileName << "\n";
                out << QString("-").repeated(40) << "\n";
                if (useConfidence) {
                    out << "[OCR FAILED - Low confidence or no text detected]\n\n";
                } else {
                    out << "[OCR FAILED - No text detected]\n\n";
                }
                out << QString("=").repeated(80) << "\n\n";

                qDebug() << "✗ OCR failed for:" << fileName;
                std::cout << "✗ OCR failed for: " << fileName.toStdString() << std::endl;
                failCount++;
            }

            // Flush the output periodically
            out.flush();
        }

        // Write summary at the end of file
        out << "\n" << QString("=").repeated(80) << "\n";
        out << "PROCESSING SUMMARY\n";
        out << QString("=").repeated(80) << "\n";
        out << "Successfully processed: " << successCount << " files\n";
        out << "Failed: " << failCount << " files\n";
        out << "Total files: " << imageFiles.count() << "\n";

        file.close();

        qDebug() << "\n=== Processing Complete ===";
        qDebug() << "Successfully processed:" << successCount << "files";
        qDebug() << "Failed:" << failCount << "files";
        qDebug() << "Total files:" << imageFiles.count();
        qDebug() << "All results saved to:" << outputFile;

        std::cout << "\n=== Processing Complete ===" << std::endl;
        std::cout << "Successfully processed: " << successCount << " files" << std::endl;
        std::cout << "Failed: " << failCount << " files" << std::endl;
        std::cout << "Total files: " << imageFiles.count() << std::endl;
        std::cout << "All results saved to: " << outputFile.toStdString() << std::endl;

        return successCount > 0;
    }

    void setTessdataPath(const QString& path) {
        tessdataPath = path;
    }

    QStringList getSupportedExtensions() const {
        return supportedExtensions;
    }

private:
    tesseract::TessBaseAPI* api;
    QString tessdataPath;
    QStringList supportedExtensions;
};

int main(int argc, char *argv[])
{
    std::cout << "=== OCR Application Starting ===" << std::endl;
    QCoreApplication app(argc, argv);

    TesseractOCR ocr;
    std::cout << "TesseractOCR object created" << std::endl;

    // Path to your folder containing images
    QString folderPath = "D:/Dataset/OCR_DATA/MLY/";
    std::cout << "Input folder path: " << folderPath.toStdString() << std::endl;

    // Check if input folder exists
    QDir inputDir(folderPath);
    if (!inputDir.exists()) {
        std::cout << "ERROR: Input folder does not exist!" << std::endl;
        std::cout << "Please check the path: " << folderPath.toStdString() << std::endl;
        return 1;
    }

    // Specify single output file
    QString outputFile = "D:/Dataset/OCR_DATA/Russian/Words/all_ocr_results_2.txt";
    std::cout << "Output file path: " << outputFile.toStdString() << std::endl;

    // Check if output directory exists
    QFileInfo outputInfo(outputFile);
    QDir outputDir = outputInfo.absoluteDir();
    if (!outputDir.exists()) {
        std::cout << "Creating output directory: " << outputDir.absolutePath().toStdString() << std::endl;
        if (!outputDir.mkpath(".")) {
            std::cout << "ERROR: Could not create output directory!" << std::endl;
            return 1;
        }
    }

    qDebug() << "Starting OCR processing for folder:" << folderPath;
    qDebug() << "Supported image formats:" << ocr.getSupportedExtensions().join(", ");

    std::cout << "Starting OCR processing for folder: " << folderPath.toStdString() << std::endl;
    std::cout << "Supported image formats: " << ocr.getSupportedExtensions().join(", ").toStdString() << std::endl;

    // Process all images in the folder and save to single file
    // Using English with confidence filtering
    bool success = ocr.processFolder(folderPath, outputFile, "rus+ukr", true, 60,6);

    if (success) {
        qDebug() << "\nOCR processing completed successfully!";
        std::cout << "\nOCR processing completed successfully!" << std::endl;
        return 0;
    } else {
        qDebug() << "\nOCR processing failed!";
        std::cout << "\nOCR processing failed!" << std::endl;
        return 1;
    }
}
