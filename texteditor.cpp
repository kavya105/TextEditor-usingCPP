#include <iostream> // For input/output
#include <vector>   // For using vectors
#include <string>   // For string manipulation
#include <stack>    // For stack operations
#include <fstream>  // For file handling
#include <sstream>  // For string stream
#include <algorithm> // For algorithms like sort, find, etc.

using namespace std;

// Structure to hold the cursor's position (line and column)
struct CursorPosition {
    int line = 0;
    int column = 0;
};

// Structure to hold formatted text with different styles
struct FormattedText {
    string text;
    bool bold = false;
    bool italic = false;
    bool underline = false;
};


// TextEditor class to handle text editing functionality
class TextEditor {
private:
    vector<FormattedText> content;
    stack<vector<FormattedText>> undoStack;
    stack<vector<FormattedText>> redoStack;
    CursorPosition cursorPosition;

public:
 // Opens a file or creates a new one if it doesn't exist
    void openFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "File not found. Do you want to create a new file named " << filename << "? (y/n): ";
            char choice;
            cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                content.clear();
                saveFile(filename); // Create a new file
                cout << "New file created: " << filename << endl;
            } else {
                cerr << "Failed to open file!" << endl;
            }
            return;
        }
        content.clear(); 
        string line;
        while (getline(file, line)) {  // Read each line from the file
            content.push_back(FormattedText{line}); // Add line to content
        }
        file.close();   // Close the file
        cursorPosition = {0, 0};      // Reset cursor position
        clearHistory();    // Clear undo/redo history
    }

    void saveFile(const string& filename) {
        ofstream file(filename);          // Open file to write
        if (!file.is_open()) {     // Check if file opened successfully 
            cerr << "Failed to save file!" << endl;
            return;
        }
        for (const FormattedText& line : content) {
            file << line.text << endl;         // Write each line of text to the file
        }
        file.close();
    }

    void displayContent() const {
        for (size_t i = 0; i < content.size(); ++i) {
            const FormattedText& line = content[i];

            // Display the line number
            cout << (i + 1) << ": ";

            // Apply formatting based on attributes
            if (line.bold) cout << "\033[1m"; // Bold
            if (line.italic) cout << "\033[3m"; // Italic
            if (line.underline) cout << "\033[4m"; // Underline

            // Simple syntax highlighting (for demonstration)
            string highlightedLine = line.text;
            size_t pos;

             // Define keywords for highlighting
            const vector<string> keywords = {"int", "return", "if", "else", "for", "while", "void"};

            // Iterate through each keyword and highlight it
            for (const string& keyword : keywords) {
                pos = highlightedLine.find(keyword);               // Find the position of the keyword
                while (pos != string::npos) {                  // While keyword is found
                    // Replace keyword with highlighted version
                    highlightedLine.replace(pos, keyword.length(), "\033[1;32m" + keyword + "\033[0m"); // Green for keywords
                    pos = highlightedLine.find(keyword, pos + keyword.length() + 11); // Account for escape sequences
                }
            }

            cout << highlightedLine; // Display the highlighted text
            cout << "\033[0m" << endl; // Reset formatting
        }
        cout << "\nCursor at line " << cursorPosition.line + 1 << ", column " << cursorPosition.column + 1 << endl;
    }

// Inserts a line of formatted text at the specified line number
    void insertLine(int lineNum, const FormattedText& formattedText) {
        if (lineNum < 1 || lineNum > content.size() + 1) {               // Check for valid line number
            cerr << "Invalid line number!" << endl;   
            return;
        }

        saveStateForUndo();      // Save the current state for undo functionality
        content.insert(content.begin() + (lineNum - 1), formattedText);  // Insert new line
        cursorPosition.line = lineNum - 1;    // Update cursor position to the new line
        cursorPosition.column = formattedText.text.size();   // Set column to end of new text
        clearRedoHistory();
    }

// Deletes a line at the specified line number
    void deleteLine(int lineNum) {
        if (lineNum < 1 || lineNum > content.size()) {
            cerr << "Invalid line number!" << endl;
            return;
        }

        saveStateForUndo();
        content.erase(content.begin() + (lineNum - 1));
        cursorPosition.line = max(0, lineNum - 2);
        cursorPosition.column = 0;
        clearRedoHistory();
    }

    void searchAndReplace(const string& searchText, const string& replaceText) {
        if (searchText.empty()) {
            cerr << "Search text cannot be empty!" << endl;
            return;
        }

        saveStateForUndo(); // Save the state for undo functionality
        bool replaced = false; // Flag to check if any replacements were made
        for (FormattedText& line : content) { // Change string& to FormattedText&
            size_t pos = line.text.find(searchText);
            while (pos != string::npos) {
                line.text.replace(pos, searchText.length(), replaceText);
                pos = line.text.find(searchText, pos + replaceText.length());
                replaced = true; // Set the flag when a replacement happens
            }
        }

        if (!replaced) {
            cout << "No instances of '" << searchText << "' were found in the document." << endl;
        } else {
            cout << "All instances of '" << searchText << "' have been replaced with '" << replaceText << "'." << endl;
        }

        clearRedoHistory(); // Clear redo history after the change
    }

    void moveCursor(char direction) {
        switch (direction) {
            case 'w': // move up
                cursorPosition.line = max(0, cursorPosition.line - 1);
                cursorPosition.column = min((int)content[cursorPosition.line].text.size(), cursorPosition.column);
                break;
            case 'a': // move left
                if (cursorPosition.column > 0) {
                    cursorPosition.column--;
                } else if (cursorPosition.line > 0) {
                    cursorPosition.line--;
                    cursorPosition.column = content[cursorPosition.line].text.size();
                }
                break;
            case 's': // move down
                if (cursorPosition.line < content.size() - 1) {
                    cursorPosition.line++;
                    cursorPosition.column = min(cursorPosition.column, (int)content[cursorPosition.line].text.size());
                }
                break;
            case 'd': // move right
                if (cursorPosition.column < content[cursorPosition.line].text.size()) {
                    cursorPosition.column++;
                } else if (cursorPosition.line < content.size() - 1) {
                    cursorPosition.line++;
                    cursorPosition.column = 0;
                }
                break;
        }
    }

    void insertCharAtCursor(char c) {
        saveStateForUndo();
        content[cursorPosition.line].text.insert(cursorPosition.column, 1, c);
        cursorPosition.column++;
        clearRedoHistory();
    }

    void deleteCharAtCursor() {
        if (cursorPosition.column == 0 && cursorPosition.line == 0) {
            cerr << "Nothing to delete!" << endl;
            return;
        }
        saveStateForUndo();
        if (cursorPosition.column > 0) {
            content[cursorPosition.line].text.erase(cursorPosition.column - 1, 1);
            cursorPosition.column--;
        } else if (cursorPosition.line > 0) {
            cursorPosition.column = content[cursorPosition.line - 1].text.size();
            content[cursorPosition.line - 1].text += content[cursorPosition.line].text; // Append the line below
            content.erase(content.begin() + cursorPosition.line); // Remove the current line
            cursorPosition.line--; // Move cursor up to the previous line
        }
        clearRedoHistory();
    }

    void undo() {
        if (!undoStack.empty()) {
            redoStack.push(content);
            content = undoStack.top();
            undoStack.pop();
        }
    }

    void redo() {
        if (!redoStack.empty()) {
            undoStack.push(content);
            content = redoStack.top();
            redoStack.pop();
        }
    }

private:
    void saveStateForUndo() {
        undoStack.push(content);
        if (undoStack.size() > 10) undoStack.pop(); // Limit undo stack size
    }

    void clearRedoHistory() {
        while (!redoStack.empty()) redoStack.pop();
    }

    void clearHistory() {
        while (!undoStack.empty()) undoStack.pop();
        while (!redoStack.empty()) redoStack.pop();
    }
};

int main() {
    TextEditor editor;
    string filename;

    cout << "Enter the filename to open: ";
    cin >> filename;

    editor.openFile(filename);
    editor.displayContent();
    while (true) {
        cout << "\nCommands: \n1. Insert line \n2. Delete line \n3. Search and replace \n4. Undo \n5. Redo \n6. Move Cursor \n7. Insert character at cursor \n8. Delete Character at Cursor \n9. Save \n10. Quit\n";
        int command;
        cin >> command;

        if (command == 1) {
            int lineNum;
            string text;
            char formatOption;
            cout << "Enter line number to insert at: ";
            cin >> lineNum;
            cout << "Enter text: ";
            cin.ignore();
            getline(cin, text);
            cout << "Is the text bold (y/n)? ";
            cin >> formatOption;
            bool bold = (formatOption == 'y' || formatOption == 'Y');

            cout << "Is the text italic (y/n)? ";
            cin >> formatOption;
            bool italic = (formatOption == 'y' || formatOption == 'Y');

            cout << "Is the text underlined (y/n)? ";
            cin >> formatOption;
            bool underline = (formatOption == 'y' || formatOption == 'Y');

            editor.insertLine(lineNum, {text, bold, italic, underline});
        } else if (command == 2) {
            int lineNum;
            cout << "Enter line number to delete: ";
            cin >> lineNum;
            editor.deleteLine(lineNum);
        } else if (command == 3) {
            string searchText, replaceText;
            cout << "Enter text to search: ";
            cin.ignore();
            getline(cin, searchText);
            cout << "Enter replacement text: ";
            getline(cin, replaceText);
            editor.searchAndReplace(searchText, replaceText);
        } else if (command == 4) {
            editor.undo();
        } else if (command == 5) {
            editor.redo();
        } else if (command == 6) {
            char direction;
            cout << "Enter direction (w/a/s/d): ";
            cin >> direction;
            editor.moveCursor(direction);
        } else if (command == 7) {
            char c;
            cout << "Enter character to insert: ";
            cin >> c;
            editor.insertCharAtCursor(c);
        } else if (command == 8) {
            editor.deleteCharAtCursor();
        } else if (command == 9) {
            editor.saveFile(filename);
            cout << "File saved!" << endl;
        } else if (command == 10) {
            break;
        } else {
            cerr << "Invalid command!" << endl;
        }
        editor.displayContent();
    }
    return 0;
}
