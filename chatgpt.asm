; The following code is written by ChatGPT

jmp start                  ; Переход к метке start

hello: bytes "Hello World!" 0  ; Нулевая строка для завершения

start:
    ; Загрузка указателя на строку
    lds hello               ; Загрузка указателя на строку в %si
    call puts               ; Вызов функции puts

    ; Завершение программы
    push 0                  ; Код выхода 0
    int 0                   ; Вызов выхода из программы

; Функция puts
puts:
.next_char:
    cmp *%si $00            ; Сравнение текущего символа с нулем
    jme .done               ; Если ноль, завершить
    push *%si               ; Сохранение символа на стеке
    int $02                 ; Вывод символа
    inx %si                 ; Переход к следующему символу
    jmp .next_char          ; Переход к следующему символу
.done:
    ret                      ; Возврат из функции

