#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>

#include "array_ptr.h"
 
// Обёртка для скрытия реального конструктора с резервированием.
struct ReserveProxyObj {
    size_t capacity_to_reserve_;
    ReserveProxyObj(size_t capacity_to_reserve) : capacity_to_reserve_(capacity_to_reserve) {}
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}
 
template <typename Type>
class SimpleVector {
 
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;
 
    SimpleVector() noexcept = default;
 
    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : SimpleVector(size, std::move(Type())) {}

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type &value) : items_(size), size_(size), capacity_(size) {
        std::fill(begin(), end(), value);
    }

    SimpleVector(size_t size, Type &&value) : items_(size), size_(size), capacity_(size) {
        for (size_t i = 0; i < size_; ++i)
        {
            items_[i] = std::move(value);
        }
    }

    // Конструктор, который будет сразу резервировать нужное количество памяти
    SimpleVector(ReserveProxyObj obj) {
    	Reserve(obj.capacity_to_reserve_);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : items_(init.size()), size_(init.size()), capacity_(init.size()) {
        std::copy(init.begin(), init.end(), begin());
    }

    // Конструктор копирования. 
    // Копия вектора должна иметь вместимость, достаточную для хранения копии элементов исходного вектора
    SimpleVector(const SimpleVector &other) : items_(other.capacity_), size_(other.size_), capacity_(other.capacity_) {
        std::copy(other.begin(), other.end(), begin());
    }
    
    // Конструктор перемещения
    SimpleVector(SimpleVector &&other) {
    	items_ = std::move(other.items_);
    	size_ = std::move(other.size_);
    	capacity_ = size_;
    	other.size_ = 0;
    	other.capacity_ = 0;
    }

    // Оператор присваивания.
    // Должен обеспечивать строгую гарантию безопасности исключений.
    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs)
        {
            SimpleVector<Type> temp(rhs);
            swap(temp);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs)
        {
            items_ = std::move(rhs.items_);
            std::swap(size_, rhs.size_);
            std::swap(capacity_, rhs.capacity_);
            rhs.size_ = 0;
            rhs.capacity_ = 0;
        }
        return *this;
    }

    // Задает ёмкость вектора
    // Если new_capacity больше текущей capacity, память должна быть перевыделена,
    // а элементы вектора скопированы в новый отрезок памяти.
    void Reserve(size_t new_capacity) {
    	if (new_capacity > capacity_) {
            ArrayPtr<Type> temp(new_capacity);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), temp.Get());
            items_.swap(temp);
            capacity_ = new_capacity;
    	}
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        auto index = size_;
        if (size_ == capacity_) {
            Resize(capacity_ + 1);
        } else {
            ++size_;
        }
        items_[index] = item;
    }

    void PushBack(Type&& item) {
        auto index = size_;
        if (size_ == capacity_) {
            Resize(capacity_ + 1);
        } else {
            ++size_;
        }
        items_[index] = std::move(item);
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type &value) {
        assert(pos >= begin() && pos <= end());
        auto index = pos - begin();
        if (size_ < capacity_) {
            std::copy_backward(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), end() + 1);
        	items_[index] = value;
        	++size_;
        } else {
            // новую вместимость SimpleVector можно выбрать как максимум из 1 (для пустого вектора) и capacity_ * 2
        	SimpleVector<Type> temp(std::max(capacity_ * 2, static_cast<size_t>(1)));
        	temp.size_ = size_ + 1;
        	std::copy(std::make_move_iterator(begin()), std::make_move_iterator(begin() + index), temp.begin());
        	temp[index] = value;
        	std::copy(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), temp.begin() + index + 1);
    	    swap(temp);
        }
        return begin() + index;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        auto index = pos - begin();
        if (size_ < capacity_) {
            std::copy_backward(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), end() + 1);
        	items_[index] = std::move(value);
        	++size_;
        } else {
            // новую вместимость SimpleVector можно выбрать как максимум из 1 (для пустого вектора) и capacity_ * 2
        	SimpleVector<Type> temp(std::max(capacity_ * 2, static_cast<size_t>(1)));
        	temp.size_ = size_ + 1;
        	std::copy(std::make_move_iterator(begin()), std::make_move_iterator(begin() + index), temp.begin());
        	temp[index] = std::move(value);
        	std::copy(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), temp.begin() + index + 1);
    	    swap(temp);
        }
        return begin() + index;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (!IsEmpty()) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        auto index = pos - begin();
        std::copy(std::make_move_iterator(begin() + index + 1), std::make_move_iterator(end()), begin() + index);
        --size_;
        return begin() + index;
    }
	
    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
    	items_.swap(other.items_);
    	std::swap(size_, other.size_);
    	std::swap(capacity_, other.capacity_);
    }
    
    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
    	return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
    	return capacity_;
    }
    
    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return !GetSize();
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
    	return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
    	return items_[index];
    }
    
    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
    	size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
    	if (new_size <= capacity_) {
        	if (new_size <= size_) {
                // сжать вектор, уменьшается размер, элементы остаются в памяти
        	    size_ = new_size;
        	} else {
                // заполнить свободные элементы по умолчанию Type{}
        	    for (size_t i = size_; i < new_size; ++i) {
                    items_[i] = std::move(Type());
                }
                size_ = new_size;
        	}
    	} else {
            // увеличить вместимость
            // новую вместимость SimpleVector можно выбрать как максимум из new_capacity и capacity_ * 2.
            capacity_ = std::max(new_size, capacity_ * 2);
            ArrayPtr<Type> temp(capacity_);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), temp.Get());
            for (size_t i = size_; i < new_size; ++i) {
                temp[i] = std::move(Type());
            }
            items_.swap(temp);
            size_ = new_size;
    	}
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        // Напишите тело самостоятельно
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};
 
template <typename Type> //основной
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type> //основной
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs > lhs);
}