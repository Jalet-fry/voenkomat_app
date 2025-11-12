# Лабораторная работа №6

**Тема:** Реализация SQL-запросов на выборку данных с использованием подзапросов, агрегатных функций, группировки и операций над множествами

**Студент:** Волосевич В.А.  
**Преподаватель:** Силич С.С  
**Минск 2025**

## ВВЕДЕНИЕ

В лабораторной работе выполняется создание запросов на выборку данных на языке SQL с использованием подзапросов, агрегатных функций, а также группировки данных (предложение GROUP BY оператора SELECT) и операций над множествами (UNION, INTERSECT, EXCEPT).

Выполнение базируется на схеме данных, спроектированной в ходе лабораторной работы №2 и имплементированной в СУБД при выполнении лабораторной работы №3.

## ИСХОДНЫЕ ДАННЫЕ

### Порядок выполнения лабораторной работы:

1. Получить у преподавателя задания по вашей собственной схеме данных, созданной в лабораторной работе №2-3 и реализованной в виде таблиц в СУБД в лабораторной работе №4. Создать запросы по заданиям (по одному запросу на каждое задание).

2. Правила выполнения заданий:

   **2.1** Для каждого задания создать реализацию в виде одного оператора SQL SELECT, в котором **можно использовать подзапросы и группировку данных**.

   **2.2** Обратить внимание, что использование скалярных (особенно соотнесенных!) подзапросов в предложении SELECT следует ограничить, т.к. они ухудшают производительность и анализ запроса, поэтому, если запрос затрагивает несколько таблиц, то сначала надо собрать данные с помощью соединения данных таблиц, и только потом выполнять их обработку (например, группировать).

   **2.3** Перед запуском запроса на выполнение, изучить данные в используемых запросом таблицах, и если требуется добавить новые данные, чтобы результат выборки не был пустым.

   **2.4** Выполнить запрос и проанализировать его результат – если есть расхождения между изученными данными и результатом запроса, то есть повод задуматься о проверке правильности выполнения этого задания.

3. Оформить отчет.

## ВЫПОЛНЕНИЕ РАБОТЫ

### 2.1 Задания

#### 2.1.1: Вывести призывников с названием категории годности и возрастом (в годах), отсортировать по возрасту (убыв.)

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    p.nomer_pasporta,
    kg.nazvanie_kategorii AS kategoria_godnosti,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS age,
    p.data_rozhdeniya
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY age DESC, p.fio;
```

#### 2.1.2: Посчитать число призывников по городам и показать только города с 1 призывником

**Скрипт:**

```sql
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(p.id_prizivnik) AS prizivniki_count
FROM public.prizivnik p
WHERE p.adres_prozhivaniya IS NOT NULL
GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
HAVING COUNT(p.id_prizivnik) = 1
ORDER BY prizivniki_count DESC;
```

#### 2.1.3: Для каждого призывника показать, сколько медицинских освидетельствований связано с ним

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    (SELECT COUNT(*) FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik) AS med_osvidetelstvovaniya_count
FROM public.prizivnik p
ORDER BY med_osvidetelstvovaniya_count DESC, p.fio;
```

#### 2.1.4: Показать призывников с их военными билетами (номер билета, воинское звание, категория)

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    vb.data_vydachi
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
ORDER BY p.fio;
```

#### 2.1.5: Посчитать количество призывников по городу

**Скрипт:**

```sql
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(*) AS prizivniki_count
FROM public.prizivnik p
WHERE p.adres_prozhivaniya IS NOT NULL
GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
ORDER BY prizivniki_count DESC;
```

#### 2.1.6: Выбрать призывников, у которых есть военный билет

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS age
FROM public.prizivnik p
WHERE EXISTS (
    SELECT 1 FROM public.voennyi_bilet vb WHERE vb.id_prizivnika = p.id_prizivnik
)
ORDER BY p.fio;
```

#### 2.1.7: Для каждого комиссара показать количество связанных призывников

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
ORDER BY prizivniki_count DESC, c.fio;
```

#### 2.1.8: Топ-5 призывников по количеству связанных с ними мероприятий

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    COALESCE(COUNT(pm.id_meropriyatiya), 0) AS meropriyatiya_count
FROM public.prizivnik p
LEFT JOIN public.prizivnik_meropriyatie pm 
    ON pm.id_prizivnik = p.id_prizivnik
GROUP BY p.id_prizivnik, p.fio
HAVING COALESCE(COUNT(pm.id_meropriyatiya), 0) > 0
ORDER BY meropriyatiya_count DESC
LIMIT 5;
```

#### 2.1.9: Найти призывников, не имеющих ни одного медицинского освидетельствования

**Скрипт:**

```sql
SELECT p.id_prizivnik, p.fio, p.data_rozhdeniya
FROM public.prizivnik p
WHERE NOT EXISTS (
    SELECT 1 FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik
)
ORDER BY p.fio;
```

#### 2.1.10: Вывести комиссаров и количество призывников в каждом

**Скрипт:**

```sql
SELECT
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    c.stazh_raboty,
    COUNT(pc.id_prizivnik) AS prizivniki_count
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio, c.dolzhnost, c.stazh_raboty
ORDER BY prizivniki_count DESC;
```

#### 2.1.11 Показать комиссаров с должностью "Военный комиссар" и отсортировать по стажу

**Скрипт:**

```sql
SELECT id_comissar, fio, dolzhnost, stazh_raboty, kontaktnyi_telefon
FROM public.comissar
WHERE dolzhnost = 'Военный комиссар'
ORDER BY stazh_raboty DESC;
```

#### 2.1.12: Найти комиссара(ов) с максимальным стажем работы

**Скрипт:**

```sql
SELECT c.id_comissar, c.fio, c.dolzhnost, c.stazh_raboty
FROM public.comissar c
WHERE c.stazh_raboty = (SELECT MAX(stazh_raboty) FROM public.comissar);
```

#### 2.1.13 Показать военные билеты вместе с ФИО призывника и возрастом призывника

**Скрипт:**

```sql
SELECT
    vb.id_bileta,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    p.fio AS prizivnik_name,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS prizivnik_age,
    vb.data_vydachi
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
ORDER BY p.fio;
```

#### 2.1.14 Подсчитать количество военных билетов по категориям годности

**Скрипт:**

```sql
SELECT kategoria, COUNT(*) AS biletov_count
FROM public.voennyi_bilet
GROUP BY kategoria
ORDER BY biletov_count DESC;
```

#### 2.1.15 Выбрать военные билеты призывников, родившихся до 2000 года

**Скрипт:**

```sql
SELECT vb.id_bileta, vb.nomer_bileta, vb.kategoria, vb.voinskoe_zvanie, vb.data_vydachi, p.id_prizivnik
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
WHERE p.data_rozhdeniya < '2000-01-01'
ORDER BY p.data_rozhdeniya;
```

#### 2.1.16 Топ-5 призывных мероприятий по дате проведения

**Скрипт:**

```sql
SELECT id_meropriyatiya, tip_meropriyatiya, data_provedeniya, mesto_provedeniya, fio_comissara
FROM public.prizivnoe_meropriyatie
ORDER BY data_provedeniya DESC
LIMIT 5;
```

#### 2.1.17 Количество и типы призывных мероприятий по каждому комиссару

**Скрипт:**

```sql
SELECT
    c.fio AS comissar_name,
    COUNT(pm.id_meropriyatiya) AS meropriyatiya_count,
    STRING_AGG(DISTINCT pm.tip_meropriyatiya, ', ' ORDER BY pm.tip_meropriyatiya) AS tipy_meropriyatii
FROM public.comissar c
LEFT JOIN public.prizivnoe_meropriyatie pm ON pm.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio
ORDER BY meropriyatiya_count DESC;
```

#### 2.1.18 Показать призывные мероприятия и связанных призывников

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

#### 2.1.19 Вывести медицинские освидетельствования с именем призывника и категорией годности

**Скрипт:**

```sql
SELECT
    mo.id_osvidetelstvovania,
    mo.data_provedeniya,
    mo.fio_vracha,
    mo.zaklyuchenie,
    p.fio AS prizivnik_name,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.med_osvidetelstvovanie mo
LEFT JOIN public.prizivnik p ON p.id_prizivnik = mo.id_prizivnika
LEFT JOIN public.kategoria_godnosti kg ON kg.id_kategorii = mo.id_kategorii
ORDER BY mo.data_provedeniya DESC;
```

#### 2.1.20 Посчитать количество медицинских освидетельствований по каждому врачу

**Скрипт:**

```sql
SELECT fio_vracha, COUNT(*) AS osvidetelstvovaniya_count
FROM public.med_osvidetelstvovanie
GROUP BY fio_vracha
ORDER BY osvidetelstvovaniya_count DESC;
```

#### 2.1.21 Показать призывников и их категории годности – и отобрать те, где категория А

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    vb.kategoria,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE vb.kategoria = 'А' OR kg.nazvanie_kategorii = 'А'
ORDER BY p.fio;
```

#### 2.1.22 Показать пары призывник-категория годности

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio AS prizivnik_name,
    vb.kategoria,
    kg.id_kategorii AS kategoria_id,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY p.fio;
```

#### 2.1.23 Сгруппировать и получить список категорий годности для каждого призывника

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    COALESCE(STRING_AGG(CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')), ', ' ORDER BY kg.id_kategorii), 'Нет категории') AS categories_list,
    COUNT(vb.id_bileta) AS categories_count
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
LEFT JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
GROUP BY p.id_prizivnik, p.fio
ORDER BY p.fio;
```

#### 2.1.24 Посчитать, сколько призывников относится к каждой категории годности

**Скрипт:**

```sql
SELECT
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    COUNT(DISTINCT p.id_prizivnik) AS prizivniki_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii
ORDER BY prizivniki_count DESC;
```

#### 2.1.25 Суммарное количество призывников по каждой категории годности

**Скрипт:**

```sql
SELECT
    kg.nazvanie_kategorii,
    COUNT(DISTINCT p.id_prizivnik) AS total_prizivniki,
    COUNT(vb.id_bileta) AS biletov_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.nazvanie_kategorii
ORDER BY total_prizivniki DESC;
```

#### 2.1.26 Показать категории годности, к которым не привязаны призывники

**Скрипт:**

```sql
SELECT 
    kg.id_kategorii, 
    kg.nazvanie_kategorii, 
    kg.index_kategorii, 
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
WHERE vb.id_bileta IS NULL
ORDER BY kg.id_kategorii;
```

#### 2.1.27 Вывести призывников, у которых категория годности с индексом больше 1

**Скрипт:**

```sql
SELECT
    p.id_prizivnik,
    p.fio,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE kg.index_kategorii IS NOT NULL AND kg.index_kategorii > 1
ORDER BY p.fio;
```

#### 2.1.28 Вывести список категорий годности с читаемым описанием

**Скрипт:**

```sql
SELECT
    kg.id_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii AS description,
    kg.osnovanie_dlya_kategorii AS basis
FROM public.kategoria_godnosti kg
ORDER BY kg.nazvanie_kategorii, COALESCE(kg.index_kategorii, 0);
```

#### 2.1.29 Вывести список категорий годности, которые используются в системе

**Скрипт:**

```sql
SELECT
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii,
    COUNT(vb.id_bileta) AS usage_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii, kg.opisanie_ogranichenii
HAVING COUNT(vb.id_bileta) > 0
ORDER BY usage_count DESC;
```

#### 2.1.30 Статистика по категориям годности (сколько призывников в каждой категории)

**Скрипт:**

```sql
SELECT 
    kg.nazvanie_kategorii,
    COUNT(DISTINCT p.id_prizivnik) AS prizivniki_count,
    ROUND(COUNT(DISTINCT p.id_prizivnik) * 100.0 / NULLIF((SELECT COUNT(*) FROM public.voennyi_bilet), 0), 2) AS percentage
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.nazvanie_kategorii
ORDER BY prizivniki_count DESC;
```

#### 2.1.31 Объединить ФИО всех участников системы (призывников и комиссаров), без дублей до 10 человек

**Скрипт:**

```sql
SELECT fio AS participant_name
FROM public.prizivnik
UNION
SELECT fio AS participant_name
FROM public.comissar
ORDER BY participant_name
LIMIT 10;
```

#### 2.1.32 Вывести призывников, у которых нет военного билета

**Скрипт:**

```sql
SELECT fio
FROM public.prizivnik
EXCEPT
SELECT p.fio
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
ORDER BY fio;
```

#### 2.1.33 Вывести города, которые встречаются и среди адресов призывников, и среди мест проведения мероприятий

**Примечание:** В отчете указан запрос из другой БД (offices, legal_entities), что является ошибкой. Правильный запрос должен использовать таблицы prizivnik и prizivnoe_meropriyatie.

#### 2.1.34 Вывести категории годности, которые используются в военных билетах и присутствуют в медицинских освидетельствованиях

**Скрипт:**

```sql
WITH common_ids AS (
    SELECT id_kategorii
    FROM public.voennyi_bilet
    WHERE id_kategorii IS NOT NULL
    
    INTERSECT
    
    SELECT id_kategorii
    FROM public.med_osvidetelstvovanie
    WHERE id_kategorii IS NOT NULL
)
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.kategoria_godnosti kg
JOIN common_ids c ON kg.id_kategorii = c.id_kategorii
ORDER BY kg.id_kategorii;
```

## ЗАКЛЮЧЕНИЕ

В процессе выполнения заданий были реализованы многоуровневые запросы с вложенными SELECT, применены агрегатные функции для вычисления показателей и структурирования данных с помощью GROUP BY. Также были использованы операции UNION, INTERSECT, EXCEPT.

В результате были выполнены полученные задания.

