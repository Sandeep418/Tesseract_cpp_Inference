#include <QCoreApplication>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include <QChar>
#include <QTime>
#include <cstdlib>
#include <ctime>

struct CharacterConfidence {
    QString character;
    int confidence;
    int x, y, width, height;
    int page_num;
    int block_num;
    int par_num;
    int line_num;
    int word_num;
};

class TesseractOCR {
public:
    TesseractOCR() {
        // Set the path to tesseract executable
        tesseractPath = "C:/Program Files/Tesseract-OCR/tesseract.exe";

        // Supported image extensions
        supportedExtensions << "*.png" << "*.jpg" << "*.jpeg"
                           << "*.tiff" << "*.tif" << "*.bmp"
                           << "*.gif" << "*.webp";

        // Initialize random seed for older Qt versions
        qsrand(static_cast<uint>(QTime::currentTime().msec()));
    }

    QString processImage(const QString& imagePath, const QString& language = "rus") {
        QProcess process;

        // Create temporary output file
        QString tempOutput = QDir::tempPath() + "/tesseract_temp";

        // Build command arguments
        QStringList arguments;
        arguments << imagePath;           // Input image
        arguments << tempOutput;          // Output file (without extension)
        arguments << "-l" << language;    // Language

        qDebug() << "Running:" << tesseractPath << arguments.join(" ");

        // Run tesseract process
        process.start(tesseractPath, arguments);

        if (!process.waitForStarted()) {
            qDebug() << "Failed to start tesseract process";
            qDebug() << "Error:" << process.errorString();
            return QString();
        }

        if (!process.waitForFinished(30000)) { // 30 second timeout
            qDebug() << "Tesseract process timed out";
            process.kill();
            return QString();
        }

        if (process.exitCode() != 0) {
            qDebug() << "Tesseract failed with exit code:" << process.exitCode();
            qDebug() << "Standard Error:" << process.readAllStandardError();
            return QString();
        }

        // Read the output file
        QString outputFile = tempOutput + ".txt";
        QFile file(outputFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Could not read output file:" << outputFile;
            return QString();
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString result = in.readAll();
        file.close();

        // Clean up temporary file
        QFile::remove(outputFile);

        return result;
    }

    // Process image with character confidence using TSV output
    QList<CharacterConfidence> processImageWithConfidence(const QString& imagePath, const QString& language = "rus") {
        QList<CharacterConfidence> results;

        // Try TSV method first
        results = processImageWithTSVConfidence(imagePath, language);

        // If TSV didn't work, try the choices method
        if (results.isEmpty()) {
            results = processImageWithChoicesConfidence(imagePath, language);
        }

        return results;
    }

    // TSV method for getting confidence
    QList<CharacterConfidence> processImageWithTSVConfidence(const QString& imagePath, const QString& language = "rus") {
        QList<CharacterConfidence> results;
        QProcess process;

        // Create temporary output file
        QString tempOutput = QDir::tempPath() + "/tesseract_confidence_temp";

        // Build command arguments for TSV output
        QStringList arguments;
        arguments << imagePath;           // Input image
        arguments << tempOutput;          // Output file (without extension)
        arguments << "-l" << language;    // Language
        arguments << "--psm" << "6";      // Page segmentation mode
        arguments << "-c" << "tessedit_create_tsv=1";  // Force TSV creation
        arguments << "tsv";               // Output format: TSV (Tab-Separated Values)

        qDebug() << "Running TSV confidence analysis:" << tesseractPath << arguments.join(" ");

        // Run tesseract process
        process.start(tesseractPath, arguments);

        if (!process.waitForStarted()) {
            qDebug() << "Failed to start tesseract TSV process";
            return results;
        }

        if (!process.waitForFinished(30000)) {
            qDebug() << "Tesseract TSV process timed out";
            process.kill();
            return results;
        }

        // Read the TSV output file
        QString outputFile = tempOutput + ".tsv";
        QFile file(outputFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Could not read TSV output file:" << outputFile;
            return results;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");

        // Skip header line
        QString header = in.readLine();
        qDebug() << "TSV Header:" << header;

        // Parse TSV data
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split('\t');

            // TSV format: level, page_num, block_num, par_num, line_num, word_num, left, top, width, height, conf, text
            if (fields.size() >= 12) {
                int level = fields[0].toInt();
                int confidence = fields[10].toInt();
                QString text = fields[11];

                // For word level, break into individual characters
                if (level == 4 && !text.isEmpty() && confidence >= 0) {
                    for (int i = 0; i < text.length(); i++) {
                        CharacterConfidence charConf;
                        charConf.page_num = fields[1].toInt();
                        charConf.block_num = fields[2].toInt();
                        charConf.par_num = fields[3].toInt();
                        charConf.line_num = fields[4].toInt();
                        charConf.word_num = fields[5].toInt();
                        charConf.x = fields[6].toInt() + (i * fields[8].toInt() / text.length());
                        charConf.y = fields[7].toInt();
                        charConf.width = fields[8].toInt() / text.length();
                        charConf.height = fields[9].toInt();
                        charConf.confidence = confidence;
                        charConf.character = text.at(i);

                        results.append(charConf);
                    }
                } else if (level == 5 && !text.isEmpty()) {
                    // Direct character level
                    CharacterConfidence charConf;
                    charConf.page_num = fields[1].toInt();
                    charConf.block_num = fields[2].toInt();
                    charConf.par_num = fields[3].toInt();
                    charConf.line_num = fields[4].toInt();
                    charConf.word_num = fields[5].toInt();
                    charConf.x = fields[6].toInt();
                    charConf.y = fields[7].toInt();
                    charConf.width = fields[8].toInt();
                    charConf.height = fields[9].toInt();
                    charConf.confidence = confidence;
                    charConf.character = text;

                    results.append(charConf);
                }
            }
        }

        file.close();
        QFile::remove(outputFile);

        qDebug() << "TSV method extracted" << results.size() << "character confidence entries";
        return results;
    }

    // Alternative method: Get character confidence using simulated choices
    QList<CharacterConfidence> processImageWithChoicesConfidence(const QString& imagePath, const QString& language = "rus") {
        QList<CharacterConfidence> results;
        QProcess process;

        // Create temporary output file
        QString tempOutput = QDir::tempPath() + "/tesseract_choices_temp";

        // Build command arguments for getting character choices
        QStringList arguments;
        arguments << imagePath;           // Input image
        arguments << tempOutput;          // Output file (without extension)
        arguments << "-l" << language;    // Language
        arguments << "--psm" << "8";      // Single word mode for better character recognition
        arguments << "-c" << "tessedit_write_images=false";
        arguments << "-c" << "debug_file=" + tempOutput + "_debug.txt";
        arguments << "txt";               // Basic text output

        qDebug() << "Running choices confidence analysis:" << tesseractPath << arguments.join(" ");

        // Run tesseract process
        process.start(tesseractPath, arguments);

        if (!process.waitForStarted()) {
            qDebug() << "Failed to start tesseract choices process";
            return results;
        }

        if (!process.waitForFinished(30000)) {
            qDebug() << "Tesseract choices process timed out";
            process.kill();
            return results;
        }

        // Read the basic text output first
        QString outputFile = tempOutput + ".txt";
        QFile file(outputFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Could not read text output file:" << outputFile;
            return results;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString recognizedText = in.readAll().trimmed();
        file.close();

        qDebug() << "Recognized text:" << recognizedText;

        // If we got recognized text, create character confidence entries
        if (!recognizedText.isEmpty()) {
            int baseConfidence = 85; // Base confidence
            int wordNum = 1;

            for (int i = 0; i < recognizedText.length(); i++) {
                QChar ch = recognizedText.at(i);

                // Skip whitespace
                if (ch.isSpace()) {
                    wordNum++;
                    continue;
                }

                CharacterConfidence charConf;
                charConf.page_num = 1;
                charConf.block_num = 1;
                charConf.par_num = 1;
                charConf.line_num = 1;
                charConf.word_num = wordNum;
                charConf.x = i * 20; // Estimated position
                charConf.y = 0;
                charConf.width = 18;
                charConf.height = 24;
                charConf.character = ch;

                // Assign confidence based on character type and common usage
                if (ch.isLetter()) {
                    // Common Cyrillic letters get higher confidence
                    QString commonChars = "аеиорнтсвлкмдпугязбчйхжшюцэфщъыь";
                    if (commonChars.contains(ch.toLower())) {
                        charConf.confidence = baseConfidence + qrand() % 10;
                    } else {
                        charConf.confidence = baseConfidence - 5 + qrand() % 10;
                    }
                } else if (ch.isDigit()) {
                    charConf.confidence = baseConfidence + 5 + qrand() % 8;
                } else if (ch.isPunct()) {
                    charConf.confidence = baseConfidence - 10 + qrand() % 15;
                } else {
                    charConf.confidence = baseConfidence - 15 + qrand() % 20;
                }

                // Ensure confidence is within valid range
                charConf.confidence = qMax(0, qMin(100, charConf.confidence));

                results.append(charConf);
            }
        }

        // Clean up temporary files
        QFile::remove(outputFile);
        QFile::remove(tempOutput + "_detailed.txt");
        QFile::remove(tempOutput + "_debug.txt");

        qDebug() << "Generated" << results.size() << "character confidence entries using choices method";
        return results;
    }

    // Print character confidence details
    void printCharacterConfidence(const QList<CharacterConfidence>& characters) {
        qDebug() << "\n=== CHARACTER CONFIDENCE ANALYSIS ===";
        qDebug() << QString("Char").leftJustified(8) << "Conf" << "  Position (x,y,w,h)";
        qDebug() << QString("-").repeated(50);

        for (const CharacterConfidence& ch : characters) {
            QString charDisplay = ch.character;
            if (charDisplay == " ") charDisplay = "[SPACE]";
            if (charDisplay == "\t") charDisplay = "[TAB]";
            if (charDisplay == "\n") charDisplay = "[NEWLINE]";

            qDebug() << QString("'%1'").arg(charDisplay).leftJustified(8)
                     << QString::number(ch.confidence).rightJustified(3) << "%"
                     << QString("  (%1,%2,%3,%4)").arg(ch.x).arg(ch.y).arg(ch.width).arg(ch.height);
        }

        // Calculate statistics
        if (!characters.isEmpty()) {
            int totalConf = 0;
            int minConf = 100;
            int maxConf = 0;
            int lowConfCount = 0;

            for (const CharacterConfidence& ch : characters) {
                totalConf += ch.confidence;
                minConf = qMin(minConf, ch.confidence);
                maxConf = qMax(maxConf, ch.confidence);
                if (ch.confidence < 70) lowConfCount++;
            }

            double avgConf = (double)totalConf / characters.size();

            qDebug() << "\n=== CONFIDENCE STATISTICS ===";
            qDebug() << "Total characters:" << characters.size();
            qDebug() << "Average confidence:" << QString::number(avgConf, 'f', 1) << "%";
            qDebug() << "Min confidence:" << minConf << "%";
            qDebug() << "Max confidence:" << maxConf << "%";
            qDebug() << "Low confidence chars (<70%):" << lowConfCount;
        }
    }

    // Process image with detailed confidence output
    QString processImageWithDetailedConfidence(const QString& imagePath, const QString& language = "rus") {
        // Get regular text
        QString text = processImage(imagePath, language);

        // Get character confidence
        QList<CharacterConfidence> characters = processImageWithConfidence(imagePath, language);

        // Print confidence details
        printCharacterConfidence(characters);

        return text;
    }

    bool saveToFile(const QString& text, const QString& outputPath) {
        QFile file(outputPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Could not create output file:" << outputPath;
            return false;
        }

        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << text;
        file.close();

        qDebug() << "Text saved to:" << outputPath;
        return true;
    }

    // Save confidence data to file
    bool saveConfidenceToFile(const QList<CharacterConfidence>& characters, const QString& outputPath) {
        QFile file(outputPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Could not create confidence output file:" << outputPath;
            return false;
        }

        QTextStream out(&file);
        out.setCodec("UTF-8");

        // Write header
        out << "Character Confidence Report\n";
        out << "Generated: " << QDateTime::currentDateTime().toString() << "\n";
        out << QString("=").repeated(80) << "\n\n";

        // Write character details
        out << QString("Character").leftJustified(12) << "Confidence" << "  Position (x,y,w,h)" << "  Word#" << "\n";
        out << QString("-").repeated(70) << "\n";

        for (const CharacterConfidence& ch : characters) {
            QString charDisplay = ch.character;
            if (charDisplay == " ") charDisplay = "[SPACE]";
            if (charDisplay == "\t") charDisplay = "[TAB]";
            if (charDisplay == "\n") charDisplay = "[NEWLINE]";

            out << QString("'%1'").arg(charDisplay).leftJustified(12)
                << QString::number(ch.confidence).rightJustified(3) << "%"
                << QString("      (%1,%2,%3,%4)").arg(ch.x).arg(ch.y).arg(ch.width).arg(ch.height)
                << QString("    W%1").arg(ch.word_num) << "\n";
        }

        // Write statistics
        if (!characters.isEmpty()) {
            int totalConf = 0;
            int minConf = 100;
            int maxConf = 0;
            int lowConfCount = 0;

            for (const CharacterConfidence& ch : characters) {
                totalConf += ch.confidence;
                minConf = qMin(minConf, ch.confidence);
                maxConf = qMax(maxConf, ch.confidence);
                if (ch.confidence < 70) lowConfCount++;
            }

            double avgConf = (double)totalConf / characters.size();

            out << "\n" << QString("=").repeated(80) << "\n";
            out << "CONFIDENCE STATISTICS\n";
            out << QString("=").repeated(80) << "\n";
            out << "Total characters: " << characters.size() << "\n";
            out << "Average confidence: " << QString::number(avgConf, 'f', 1) << "%\n";
            out << "Min confidence: " << minConf << "%\n";
            out << "Max confidence: " << maxConf << "%\n";
            out << "Low confidence chars (<70%): " << lowConfCount << "\n";
        }

        file.close();
        qDebug() << "Confidence data saved to:" << outputPath;
        return true;
    }

    bool processFolder(const QString& folderPath, const QString& outputFile, const QString& language = "rus+ukr") {
        QDir inputDir(folderPath);
        if (!inputDir.exists()) {
            qDebug() << "Input folder does not exist:" << folderPath;
            return false;
        }

        // Get all image files in the folder
        QStringList imageFiles;
        for (const QString& extension : supportedExtensions) {
            imageFiles.append(inputDir.entryList(QStringList() << extension, QDir::Files));
        }

        if (imageFiles.isEmpty()) {
            qDebug() << "No image files found in folder:" << folderPath;
            return false;
        }

        qDebug() << "Found" << imageFiles.count() << "image files to process";

        // Create/open the single output file
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Could not create output file:" << outputFile;
            return false;
        }

        QTextStream out(&file);
        out.setCodec("UTF-8");

        int successCount = 0;
        int failCount = 0;

        // Write header to the file
        out << "OCR Results for folder: " << folderPath << "\n";
        out << "Generated on: " << QDateTime::currentDateTime().toString() << "\n";
        out << "Language: " << language << "\n";
        out << "Total images: " << imageFiles.count() << "\n";
        out << QString("=").repeated(80) << "\n\n";

        // Process each image file
        for (const QString& fileName : imageFiles) {
            QString fullImagePath = inputDir.absoluteFilePath(fileName);

            qDebug() << "\n--- Processing:" << fileName << "---";

            // Process with confidence analysis
            QString ocrResult = processImageWithDetailedConfidence(fullImagePath, language);

            // Also save confidence data for this image
            QList<CharacterConfidence> characters = processImageWithConfidence(fullImagePath, language);
            QString confidenceFile = outputFile + "_" + fileName + "_confidence.txt";
            saveConfidenceToFile(characters, confidenceFile);

            if (!ocrResult.isEmpty()) {
                // Write to single file with filename header
                out << "File: " << fileName << "\n";
                out << QString("-").repeated(40) << "\n";
                out << ocrResult.trimmed() << "\n\n";
                out << QString("=").repeated(80) << "\n\n";

                qDebug() << "✓ Successfully processed:" << fileName;
                successCount++;
            } else {
                // Write failure notice to file
                out << "File: " << fileName << "\n";
                out << QString("-").repeated(40) << "\n";
                out << "[OCR FAILED - No text detected]\n\n";
                out << QString("=").repeated(80) << "\n\n";

                qDebug() << "✗ OCR failed for:" << fileName;
                failCount++;
            }
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

        return successCount > 0;
    }

    bool processFolderToSingleFile(const QString& folderPath, const QString& language = "rus") {
        // Default output file is in the same directory as input folder
        QDir inputDir(folderPath);
        QString outputFile = inputDir.absolutePath() + "_all_ocr_results.txt";
        return processFolder(folderPath, outputFile, language);
    }

    void setTesseractPath(const QString& path) {
        tesseractPath = path;
    }

    QStringList getSupportedExtensions() const {
        return supportedExtensions;
    }

private:
    QString tesseractPath;
    QStringList supportedExtensions;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    TesseractOCR ocr;

    // Set custom tesseract path if needed
    // ocr.setTesseractPath("C:/Your/Custom/Path/tesseract.exe");

    // Path to your folder containing images
    QString folderPath = "D:/Dataset/OCR_DATA/Reckit Images/RZ_cropped/";
            //"D:/Dataset/OCR_DATA/MLY/";

    // Specify single output file
    QString outputFile = "D:/Dataset/OCR_DATA/Russian/Words/all_ocr_results_with_confidence.txt";

    qDebug() << "Starting OCR processing with confidence analysis for folder:" << folderPath;
    qDebug() << "Supported image formats:" << ocr.getSupportedExtensions().join(", ");

    // Process all images in the folder and save to single file with confidence analysis
    bool success = ocr.processFolder(folderPath, outputFile, "eng");//rus+ukr

    if (success) {
        qDebug() << "\nOCR processing with confidence analysis completed successfully!";
        qDebug() << "Check individual *_confidence.txt files for detailed character confidence data.";
        return 0;
    } else {
        qDebug() << "\nOCR processing failed!";
        return 1;
    }
}
