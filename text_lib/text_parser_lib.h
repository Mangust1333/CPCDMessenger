#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <sys/stat.h>
#include <cstdlib>

namespace fs = std::filesystem;

bool change_permissions(const std::string& path, const std::string& mode) {
    std::string command = "chmod " + mode + " " + path;
    int result = std::system(command.c_str());
    return result == 0;
}

// Функция для получения текущих прав доступа
std::string get_current_permissions(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        throw std::runtime_error("Не удалось получить права доступа к файлу");
    }

    // Формируем строку с правами доступа в восьмеричном формате
    char permissions[10];
    snprintf(permissions, sizeof(permissions), "%o", info.st_mode & 0777);
    return std::string(permissions);
}

void create_and_write_file(const std::string& text, const std::string& file_path) {
    std::string original_permissions;
    fs::path dir_path = fs::path(file_path).parent_path();
    if (fs::exists(dir_path)) {
        original_permissions = get_current_permissions(dir_path.string());
    }
    try {

        // Создаем директорию, если она не существует
        if (!fs::exists(dir_path)) {
            fs::create_directories(dir_path);
        }

        // Пытаемся открыть файл для записи
        std::ofstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл для записи");
        }

        // Записываем текст в файл
        file << text;

        // Закрываем файл
        file.close();

        std::cout << "Файл успешно создан и запись завершена: " << file_path << std::endl;

        // Восстанавливаем права доступа директории
        if (!original_permissions.empty()) {
            change_permissions(dir_path.string(), original_permissions);
        }

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Ошибка файловой системы: " << e.what() << std::endl;

        // Попытка изменить права доступа к директории
        if (change_permissions(dir_path.string(), "777")) {
            std::cerr << "Права доступа к директории изменены. Повторная попытка..." << std::endl;

            // Повторная попытка создания и записи файла после изменения прав
            create_and_write_file(text, file_path);
        } else {
            std::cerr << "Не удалось изменить права доступа к директории: " << dir_path.string() << std::endl;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;

        // Попытка изменить права доступа к файлу, если файл был создан, но не открыт
        if (fs::exists(file_path) && change_permissions(file_path, "777")) {
            std::cerr << "Права доступа к файлу изменены. Повторная попытка..." << std::endl;

            // Повторная попытка создания и записи файла после изменения прав
            create_and_write_file(text, file_path);
        } else {
            std::cerr << "Не удалось изменить права доступа к файлу: " << file_path << std::endl;
        }
    }

    // Восстанавливаем права доступа файла, если они были изменены
    if (fs::exists(file_path)) {
        std::string file_permissions = get_current_permissions(file_path);
        if (file_permissions != "777") {
            change_permissions(file_path, original_permissions);
        }
    }
}

int main() {
    std::string text = "Пример текста для записи в файл.";
    std::string file_path = "/путь/к/файлу/файл.txt";

    create_and_write_file(text, file_path);

    return 0;
}
