# Автоматное программирование

В написании использовался редактор `Atom`, а в качестве генератора `pdf` [DILLINGER](https://dillinger.io/)

## Необходимо знать

Для освоение материала курса, студенту необходимо иметь начальные знания:
1. Архитектура ЭВМ;
2. Программирование на ЯПВУ Си;
3. Дискретная математика.

## Цель курса

## Описание

1. [История](00.History/00.History.md). Тьюринг, тезисы Черча. Вычислимость. Парадигмы программирования. История языка Си. С++ и Бьярн Страуструп с обобщенным программированием, как логическое продолжение языка. Стандарты языка. `POSIX`. Почему язык Си и почему стандарт `C89`(индекс [TIOBE](https://www.tiobe.com/tiobe-index)).
2. [Окружение](01.Environment/01.Environment.md). Окружение сборки. Окружение разработки. `IDE`, `JetBrains CLion`, `Microsoft Visual Studio Community Edition`. `MinGW-64`. Проект. Структура проекта. Автоматизация сборки. `cmake`. Непрерывная интеграция.
3. [Системы управления версиями](02.VerControlSystem/02.VerControlSystem.md). `CVS`, `SVN`, `Mercurial`, `Git`. Базовые команды. `Index`, `Commit`, `Revert`, `Merge`, `Rebase`, `Pull`, `Push`, `Branch`, `Tag`. Последовательность работы. Разрешение конфликтов.
4. [Абстрактные вычислительные машины](03.Turing_Machine/03.Turing_Machine.md) Машина Тьюринга. Машина Поста.
5. [Регулярные выражения](04.Regular_Expression/04.Regular_Expression.md) Регулярные выражения. Описание. Переход от регулярного выражения к автомату.

## Порядок приема лабораторных работ

1. Проверка кода на соответствие [Стилю](-1.CodeStyle/-1.CodeStyle.md)

## Порядок проведения практических работ

##Задания КР
1. Разработка автоматной модели установки изделия на конвейер
2. Разработка автоматной модели установки деталей на изделие
3. Разработка автоматной модели сварки деталей на изделии
4. Разработка автоматного алгоритма поиска и проверки правильности установки деталей
5. Разработка автоматной модели покраски изделий
6. Разработка автоматной модели сортировки и упаковки изделий

## 

### Первые вопросы МТ
Построить МТ (таблицу переходов) + построить граф переходов (автомат)

1. На ленте машины Тьюринга содержится последовательность символов “+”. Напишите программу для машины Тьюринга, которая каждый второй символ “+” заменит на “–”. Замена начинается с правого конца последовательности. Автомат в состоянии q1 обозревает один из символов указанной последовательности. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
2. Дано число n в восьмеричной системе счисления. Разработать машину Тьюринга, которая увеличивала бы заданное число n на 1. Автомат в состоянии q1 обозревает некую цифру входного слова. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
3. Дана десятичная запись натурального числа n > 1. Разработать машину Тьюринга, которая уменьшала бы заданное число n на 1. Автомат в состоянии q1 обозревает правую цифру числа. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
4. Дано натуральное число n > 1. Разработать машину Тьюринга, которая уменьшала бы заданное число n на 1, при этом в выходном слове старшая цифра не должна быть 0. Например, если входным словом было “100”, то выходным словом должно быть “99”, а не “099”. Автомат в состоянии q1 обозревает правую цифру числа. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
5. Дан массив из открывающих и закрывающих скобок. Построить машину Тьюринга, которая удаляла бы пары взаимных скобок, т.е. расположенных подряд “( )”. Например, дано “) ( ( ) ( ( )”, надо получить “) . . . ( ( ”. Автомат в состоянии q1 обозревает крайний левый символ строки. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
6. Дана строка из букв “a” и “b”. Разработать машину Тьюринга, которая переместит все буквы “a” в левую, а буквы “b” — в правую части строки. Автомат в состоянии q1 обозревает крайний левый символ строки. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
7. На ленте машины Тьюринга находится число, записанное в десятичной системе счисления. Умножить это число на 2. Автомат в состоянии q1 обозревает крайнюю левую цифру числа. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
8. Даны два натуральных числа m и n, представленные в унарной системе счисления. Соответствующие наборы символов “|” разделены пустой клеткой. Автомат в состоянии q1обозревает самый правый символ входной последовательности. Разработать машину Тьюринга, которая на ленте оставит сумму чисел m и n. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
9. Даны два натуральных числа m и n, представленных в унарной системе счисления. Соответствующие наборы символов “|” разделены пустой клеткой. Автомат в состоянии q1 обозревает самый правый символ входной последовательности. Разработать машину Тьюринга, которая на ленте оставит разность чисел m и n. Известно, что m > n. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
10. На ленте машины Тьюринга находится десятичное число. Определить, делится ли это число на 5 без остатка. Если делится, то записать справа от числа слово “да”, иначе — “нет”. Автомат обозревает некую цифру входного числа. Кроме самой программы-таблицы, описать словами, что выполняется машиной в каждом состоянии.
11. A = {a, b, c}. Если  первый  и  последний  символы  (непустого)  слова Р одинаковы, тогда это слово не менять, а иначе заменить его на пустое слово
12. A = {a, b, c}. Удалить из слова Р его второй символ, если такой есть.
13. A = {a, b, c}. Удалить из слова Р первое вхождение символа a , если такое есть 
14. A = {a, b, c}. Если Р - непустое слово, то за его первым символом вставить символ a
15. A = {a, b, c}. Вставить в слово P символ a за первым вхождением символа c, если такое есть
16. A = {a, b, c}. Удалить из P все вхождения слова а
17. A = {a, b}. Удвоить слово P поставив разделитель = между ними
18. A = {a, b, c}. Приписать к словне пустому слову Р, е его начало первый символ
19. A = {a, b, c}. Заменить в слове Р слова ab на символ с
21. A = {a, b, c}. Перевернуть слово P. Например aab -> baa
22. A = {a, b}. Удвоить каждый символ Р. aba -> aabbaa

### Регулярные выражения

Описать резулярное выражение исходя из условия + построить по регулярному выражению автомат


## Задание выдано

02.11.2018
