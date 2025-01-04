import os

def count_lines(file_path):
    """
    Count the number of source lines of code (SLOC) in a file, excluding blank lines and comments.
    """
    sloc = 0
    multi_line_comment = False
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            stripped_line = line.strip()
            
            # Handle multi-line comments
            if multi_line_comment:
                if '*/' in stripped_line:
                    multi_line_comment = False
                continue
            
            if '/*' in stripped_line:
                multi_line_comment = True
                continue
            
            # Skip single-line comments and blank lines
            if stripped_line.startswith('//') or not stripped_line:
                continue
            
            # Count valid lines of code
            sloc += 1
    return sloc

def calculate_sloc(directory):
    """
    Traverse the directory and calculate total SLOC for all source files.
    """
    total_sloc = 0
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cpp', '.c', '.java', '.py', '.js')):  # Add file extensions as needed
                file_path = os.path.join(root, file)
                sloc = count_lines(file_path)
                print(f"{file}: {sloc} lines of code")
                total_sloc += sloc
    return total_sloc

if __name__ == "__main__":
    directory = input("Enter the directory to scan for source code: ").strip()
    if not os.path.exists(directory):
        print("The directory does not exist.")
    else:
        total_sloc = calculate_sloc(directory)
        print(f"\nTotal SLOC in directory '{directory}': {total_sloc}")
