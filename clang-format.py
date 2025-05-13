import os
import subprocess

def clang_format_file(file_path, clang_format_path=".clang-format"):
    # Форматируем файл с помощью clang-format и заданного .clang-format
    try:
        subprocess.run(["clang-format", "-i", "-style=file", "-assume-filename=" + clang_format_path, file_path], check=True)
        print(f"Файл {file_path} успешно отформатирован.")
    except subprocess.CalledProcessError as e:
        print(f"Ошибка при форматировании файла {file_path}: {e}")

def clang_format_directory(directory, clang_format_path=".clang-format"):
    # Пробегаемся по всем файлам в директории и поддиректориям
    for root, dirs, files in os.walk(directory):
        for file in files:
            # Фильтруем только файлы с расширением .cpp или .h
            if file.endswith((".cpp", ".hpp")):
                file_path = os.path.join(root, file)
                clang_format_file(file_path, clang_format_path)

# Замените 'путь/к/директории' на путь к вашей директории с файлами C++
clang_format_directory('.', './clang-format')