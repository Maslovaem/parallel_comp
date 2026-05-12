import numpy as np
import matplotlib.pyplot as plt
import sys
import os

def plot_from_file(filename):
    """Читает данные из файла и строит график решения."""
    if not os.path.exists(filename):
        print(f"Ошибка: Файл '{filename}' не найден.")
        return

    try:
        # Загружаем данные, пропуская первую строку (комментарий)
        data = np.loadtxt(filename, comments='#')
        x = data[:, 0]
        u_numerical = data[:, 1]
        u_exact = data[:, 2]
    except Exception as e:
        print(f"Ошибка при чтении файла '{filename}': {e}")
        return

    plt.figure(figsize=(10, 6))
    plt.plot(x, u_numerical, 'b-', label='Численное решение')
    plt.plot(x, u_exact, 'r--', label='Точное решение')
    plt.xlabel('Координата x')
    plt.ylabel('u(T_END, x)')
    plt.title(f'Сравнение решений из файла: {os.path.basename(filename)}')
    plt.legend()
    plt.grid(True)
    plt.ylim(-1.1, 1.1) # Ограничим ось Y для наглядности синусоиды
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Использование: python plot_solution.py <имя_файла_данных>")
        print("Пример: python plot_solution.py solution_seq.dat")
        sys.exit(1)

    data_filename = sys.argv[1]
    plot_from_file(data_filename)
