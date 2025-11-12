# Лабораторная работа №5

**Тема:** Реализация SQL-запросов на выборку данных с использованием подзапросов, агрегатных функций, группировки и операций над множествами

**Студент:** Волосевич В.А.  
**Преподаватель:** Силич С.С  
**Минск 2025**

## ВВЕДЕНИЕ

В лабораторной работе выполняется создание простых запросов на выборку данных на языке SQL с использованием предложений SELECT, FROM (JOINS), WHERE и ORDER BY оператора SELECT. В работе также требуется рассмотреть использование скалярных функций.

Выполнение базируется на схеме данных, спроектированной в ходе лабораторной работы №2 и имплементированной в СУБД при выполнении лабораторной работы №3.

## ИСХОДНЫЕ ДАННЫЕ

### Порядок выполнения лабораторной работы:

1. Получить у преподавателя задания по вашей собственной схеме данных, созданной в лабораторной работе №2-3 и реализованной в виде таблиц в СУБД в лабораторной работе №4. Создать запросы по заданиям (по одному запросу на каждое задание).

2. Правила выполнения заданий:

   **2.1** Для каждого задания создать реализацию в виде одного оператора выборки, в котором **НЕЛЬЗЯ использовать подзапросы и группировку данных** (это еще будет в другой лабораторной работе);

   **2.2** При использовании соединений нескольких таблиц обратить внимание на условие задания и сделать выбор между внутренним и внешним соединениями и их вариантами реализации;

   **2.3** Перед запуском запроса на выполнение, изучить данные в используемых запросом таблицах, и, если требуется, добавить в вашу схему необходимые новые данные, чтобы результат выборки был контролируемым и не пустым;

   **2.4** Выполнить запрос и проанализировать его результат – если есть расхождения между ожидаемыми данными и результатом запроса, то есть повод задуматься о проверке правильности выполнения этого задания.

3. Оформить отчет.

## ВЫПОЛНЕНИЕ РАБОТЫ

### 2.1 Задания

#### 2.1.1 Вывести призывников с ФИО, датой рождения и возрастом, отсортировать по возрасту по убыванию.

**Скрипт:**

```sql
SELECT 
    id_prizivnik,
    fio,
    data_rozhdeniya,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, data_rozhdeniya)) AS age
FROM public.prizivnik
ORDER BY age DESC;
```

#### 2.1.2 Посчитать количество призывников по городам проживания.

**Скрипт:**

```sql
SELECT 
    TRIM(SPLIT_PART(adres_prozhivaniya, ',', 1)) AS city,
    COUNT(*) AS prizivniki_count
FROM public.prizivnik
GROUP BY city
ORDER BY prizivniki_count DESC;
```

#### 2.1.3 Вывести призывников старше 20 лет с форматированием ФИО и номера паспорта.

**Скрипт:**

```sql
SELECT 
    id_prizivnik,
    fio,
    nomer_pasporta,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, data_rozhdeniya)) AS age,
    CONCAT(fio, ' — паспорт: ', nomer_pasporta) AS info
FROM public.prizivnik
WHERE EXTRACT(YEAR FROM AGE(CURRENT_DATE, data_rozhdeniya)) > 20
ORDER BY age DESC, fio;
```

#### 2.1.4 Вывести призывников с их военными билетами.

**Скрипт:**

```sql
SELECT 
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
ORDER BY p.fio;
```

#### 2.1.5 Посчитать количество призывников по воинским званиям

**Скрипт:**

```sql
SELECT 
    vb.voinskoe_zvanie,
    COUNT(p.id_prizivnik) AS count_prizivniki
FROM public.voennyi_bilet vb
LEFT JOIN public.prizivnik p ON p.id_voennogo_bileta = vb.id_bileta
GROUP BY vb.voinskoe_zvanie
ORDER BY count_prizivniki DESC;
```

#### 2.1.6 Вывести всех призывников с категорией годности А.

**Скрипт:**

```sql
SELECT 
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    vb.kategoria,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE vb.kategoria = 'А'
ORDER BY p.fio;
```

#### 2.1.7 Для каждого комиссара показать количество связанных призывников.

**Скрипт:**

```sql
SELECT 
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    COUNT(pc.id_prizivnik) AS prizivniki_count
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio, c.dolzhnost
ORDER BY prizivniki_count DESC;
```

#### 2.1.8 Топ-5 комиссаров по стажу работы.

**Скрипт:**

```sql
SELECT 
    id_comissar,
    fio,
    dolzhnost,
    stazh_raboty,
    kontaktnyi_telefon
FROM public.comissar
WHERE stazh_raboty IS NOT NULL
ORDER BY stazh_raboty DESC
LIMIT 5;
```

#### 2.1.9 Вывести комиссаров с должностью "Военный комиссар" и отсортировать по стажу.

**Скрипт:**

```sql
SELECT 
    id_comissar,
    fio,
    dolzhnost,
    stazh_raboty,
    kontaktnyi_telefon
FROM public.comissar
WHERE dolzhnost = 'Военный комиссар'
ORDER BY stazh_raboty DESC;
```

#### 2.1.10 Список комиссаров с количеством призывников в каждом (включая комиссаров без призывников).

**Скрипт:**

```sql
SELECT 
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    COUNT(pc.id_prizivnik) AS prizivniki_count
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio, c.dolzhnost
ORDER BY prizivniki_count DESC;
```

#### 2.1.11 Показать комиссаров и скомпонованный список призывников.

**Скрипт:**

```sql
SELECT 
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    STRING_AGG(p.fio, ', ' ORDER BY p.fio) AS prizivniki_list
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
LEFT JOIN public.prizivnik p ON p.id_prizivnik = pc.id_prizivnik
GROUP BY c.id_comissar, c.fio, c.dolzhnost
ORDER BY c.fio;
```

#### 2.1.12 Вывести комиссаров со стажем работы более 10 лет, упорядочить по стажу.

**Скрипт:**

```sql
SELECT 
    id_comissar,
    fio,
    dolzhnost,
    stazh_raboty,
    kontaktnyi_telefon
FROM public.comissar
WHERE stazh_raboty > 10
ORDER BY stazh_raboty DESC;
```

#### 2.1.13 Показать военные билеты вместе с ФИО призывника и возрастом.

**Скрипт:**

```sql
SELECT 
    vb.id_bileta,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    p.fio AS prizivnik_name,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS prizivnik_age
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
ORDER BY p.fio;
```

#### 2.1.14 Вывести военные билеты, выданные после 2023-01-01, с сортировкой по дате выдачи.

**Скрипт:**

```sql
SELECT 
    id_bileta,
    nomer_bileta,
    voinskoe_zvanie,
    kategoria,
    data_vydachi
FROM public.voennyi_bilet
WHERE data_vydachi > '2023-01-01'
ORDER BY data_vydachi;
```

#### 2.1.15 Подсчитать количество военных билетов по категориям.

**Скрипт:**

```sql
SELECT 
    kategoria,
    COUNT(*) AS biletov_count
FROM public.voennyi_bilet
GROUP BY kategoria
ORDER BY biletov_count DESC;
```

#### 2.1.16 Топ-5 призывных мероприятий по дате проведения.

**Скрипт:**

```sql
SELECT 
    id_meropriyatiya,
    tip_meropriyatiya,
    data_provedeniya,
    mesto_provedeniya,
    fio_comissara
FROM public.prizivnoe_meropriyatie
ORDER BY data_provedeniya DESC
LIMIT 5;
```

#### 2.1.17 Количество и типы призывных мероприятий по каждому комиссару.

**Скрипт:**

```sql
SELECT 
    c.fio AS comissar_name,
    COUNT(pm.id_meropriyatiya) AS meropriyatiya_count,
    STRING_AGG(DISTINCT pm.tip_meropriyatiya, ', ') AS tipy_meropriyatii
FROM public.comissar c
LEFT JOIN public.prizivnoe_meropriyatie pm ON pm.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio
ORDER BY meropriyatiya_count DESC;
```

#### 2.1.18 Показать призывные мероприятия и связанных призывников.

**Скрипт:**

```sql
SELECT 
    pm.id_meropriyatiya,
    pm.tip_meropriyatiya,
    pm.data_provedeniya,
    pm.mesto_provedeniya,
    p.fio AS prizivnik_name
FROM public.prizivnoe_meropriyatie pm
JOIN public.prizivnik_meropriyatie pmm ON pmm.id_meropriyatiya = pm.id_meropriyatiya
JOIN public.prizivnik p ON p.id_prizivnik = pmm.id_prizivnik
ORDER BY pm.id_meropriyatiya;
```

#### 2.1.19 Вывести медицинские освидетельствования с именем призывника и категорией годности.

**Скрипт:**

```sql
SELECT 
    mo.id_osvidetelstvovania,
    mo.data_provedeniya,
    mo.fio_vracha,
    mo.zaklyuchenie,
    p.fio AS prizivnik_name,
    kg.nazvanie_kategorii,
    kg.opisanie_ogranichenii
FROM public.med_osvidetelstvovanie mo
JOIN public.prizivnik p ON p.id_prizivnik = mo.id_prizivnika
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = mo.id_kategorii
ORDER BY mo.data_provedeniya DESC;
```

#### 2.1.20 Посчитать количество медицинских освидетельствований по каждому врачу.

**Скрипт:**

```sql
SELECT 
    fio_vracha,
    COUNT(*) AS osvidetelstvovaniya_count
FROM public.med_osvidetelstvovanie
GROUP BY fio_vracha
ORDER BY osvidetelstvovaniya_count DESC;
```

#### 2.1.21 Для каждой категории годности вывести количество, средний возраст призывников.

**Скрипт:**

```sql
SELECT 
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    COUNT(p.id_prizivnik) AS prizivniki_count,
    ROUND(AVG(EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)))::numeric, 2) AS avg_age
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii
ORDER BY kg.nazvanie_kategorii, kg.index_kategorii;
```

#### 2.1.22 Показать пары призывник-категория годности.

**Скрипт:**

```sql
SELECT 
    p.id_prizivnik,
    p.fio AS prizivnik_name,
    vb.kategoria,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    kg.opisanie_ogranichenii
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY p.fio;
```

#### 2.1.23 Сгруппировать и получить список категорий годности для каждого призывника.

**Скрипт:**

```sql
SELECT 
    p.id_prizivnik,
    p.fio AS prizivnik_name,
    STRING_AGG(CONCAT(kg.nazvanie_kategorii, kg.index_kategorii), ', ' ORDER BY kg.id_kategorii) AS categories_list,
    COUNT(vb.id_bileta) AS categories_count
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
LEFT JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
GROUP BY p.id_prizivnik, p.fio
ORDER BY p.fio;
```

#### 2.1.24 Посчитать, сколько призывников относится к каждой категории годности.

**Скрипт:**

```sql
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    COUNT(DISTINCT p.id_prizivnik) AS prizivniki_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii
ORDER BY prizivniki_count DESC;
```

#### 2.1.25 Суммарное количество призывников по каждой категории годности.

**Примечание:** В отчете указан запрос из другой БД (tax_return_payment_order), что является ошибкой. Правильный запрос должен быть аналогичен 2.1.24.

#### 2.1.26 Показать категории годности, к которым не привязаны призывники.

**Примечание:** В отчете указан запрос, идентичный 2.1.22, что является ошибкой. Правильный запрос:

```sql
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
WHERE vb.id_bileta IS NULL
ORDER BY kg.id_kategorii;
```

#### 2.1.27 Показать военные билеты вместе с полной информацией о категории годности.

**Примечание:** В отчете указан запрос, идентичный 2.1.23, что является ошибкой.

#### 2.1.28 Вывести список категорий годности с читаемым описанием.

**Скрипт:**

```sql
SELECT 
    kg.id_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    kg.opisanie_ogranichenii AS description,
    kg.osnovanie_dlya_kategorii AS basis
FROM public.kategoria_godnosti kg
ORDER BY kg.nazvanie_kategorii, kg.index_kategorii;
```

#### 2.1.29 Вывести список категорий годности, которые используются в системе.

**Скрипт:**

```sql
SELECT 
    kg.nazvanie_kategorii,
    COUNT(vb.id_bileta) AS total_prizivniki,
    COUNT(DISTINCT vb.id_prizivnika) AS unique_prizivniki
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
GROUP BY kg.nazvanie_kategorii
ORDER BY total_prizivniki DESC;
```

#### 2.1.30 Статистика по категориям годности (сколько призывников в каждой категории).

**Скрипт:**

```sql
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    kg.opisanie_ogranichenii
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
WHERE vb.id_bileta IS NULL
ORDER BY kg.id_kategorii;
```

## ЗАКЛЮЧЕНИЕ

В процессе выполнения заданий были реализованы многоуровневые запросы с вложенными SELECT, применены агрегатные функции для вычисления показателей и структурирования данных с помощью GROUP BY.

В результате были выполнены полученные задания.

