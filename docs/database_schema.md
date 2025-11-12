# Описание схемы базы данных "Военкомат"

## Общая информация

- **Название БД:** voenkomat
- **СУБД:** PostgreSQL 17.6
- **Схема:** public
- **Владелец:** postgres

## Структура базы данных

База данных содержит 9 таблиц, связанных между собой через внешние ключи (FOREIGN KEY).

### Таблицы

#### 1. `comissar` (Комиссары)

Хранит информацию о военных комиссарах.

**Поля:**
- `id_comissar` (integer, PRIMARY KEY, AUTO_INCREMENT) - Уникальный идентификатор комиссара
- `fio` (varchar(255), NOT NULL) - ФИО комиссара
- `dolzhnost` (varchar(100), NOT NULL) - Должность
- `stazh_raboty` (integer) - Стаж работы (в годах)
- `kontaktnyi_telefon` (varchar(20)) - Контактный телефон

**Связанные таблицы:**
- `prizivnik_comissar` - связь многие-ко-многим с призывниками
- `prizivnoe_meropriyatie` - связь один-ко-многим с мероприятиями

#### 2. `kategoria_godnosti` (Категории годности)

Хранит категории годности к военной службе.

**Поля:**
- `id_kategorii` (integer, PRIMARY KEY, AUTO_INCREMENT) - Уникальный идентификатор категории
- `nazvanie_kategorii` (varchar(1), NOT NULL) - Название категории (А, Б, В, Г, Д)
- `opisanie_ogranichenii` (text) - Описание ограничений
- `index_kategorii` (integer) - Индекс категории (1, 2, 3)
- `osnovanie_dlya_kategorii` (text) - Основание для категории

**Связанные таблицы:**
- `voennyi_bilet` - связь один-ко-многим
- `med_osvidetelstvovanie` - связь один-ко-многим

#### 3. `med_osvidetelstvovanie` (Медицинские освидетельствования)

Хранит информацию о медицинских освидетельствованиях призывников.

**Поля:**
- `id_osvidetelstvovania` (integer, PRIMARY KEY, AUTO_INCREMENT) - Уникальный идентификатор освидетельствования
- `data_provedeniya` (date, NOT NULL) - Дата проведения
- `rezultaty_obsledovania` (text) - Результаты обследования
- `fio_vracha` (varchar(255), NOT NULL) - ФИО врача
- `zaklyuchenie` (text) - Заключение
- `id_prizivnika` (integer, NOT NULL, FOREIGN KEY -> prizivnik) - Ссылка на призывника
- `id_kategorii` (integer, NOT NULL, FOREIGN KEY -> kategoria_godnosti) - Ссылка на категорию годности

**Связанные таблицы:**
- `prizivnik` - связь многие-к-одному
- `kategoria_godnosti` - связь многие-к-одному

#### 4. `prizivnik` (Призывники)

Основная таблица с информацией о призывниках.

**Поля:**
- `id_prizivnik` (integer, PRIMARY KEY, AUTO_INCREMENT) - Уникальный идентификатор призывника
- `fio` (varchar(255), NOT NULL) - ФИО призывника
- `data_rozhdeniya` (date, NOT NULL) - Дата рождения
- `adres_prozhivaniya` (text) - Адрес проживания
- `nomer_pasporta` (varchar(20), NOT NULL) - Номер паспорта
- `id_voennogo_bileta` (integer, FOREIGN KEY -> voennyi_bilet) - Ссылка на военный билет
- `id_voenno_uchetnoi_karty` (integer, FOREIGN KEY -> voenno_uchetnaya_karta) - Ссылка на военно-учётную карту

**Связанные таблицы:**
- `voennyi_bilet` - связь один-к-одному
- `voenno_uchetnaya_karta` - связь один-к-одному
- `prizivnik_comissar` - связь многие-ко-многим с комиссарами
- `prizivnik_meropriyatie` - связь многие-ко-многим с мероприятиями
- `med_osvidetelstvovanie` - связь один-ко-многим

#### 5. `prizivnik_comissar` (Связь призывник-комиссар)

Связующая таблица для связи многие-ко-многим между призывниками и комиссарами.

**Поля:**
- `id_prizivnik` (integer, NOT NULL, PRIMARY KEY, FOREIGN KEY -> prizivnik) - Ссылка на призывника
- `id_comissar` (integer, NOT NULL, PRIMARY KEY, FOREIGN KEY -> comissar) - Ссылка на комиссара
- `data_vzaimodeistviya` (date) - Дата взаимодействия
- `nomer_kabineta` (varchar(10)) - Номер кабинета

**Составной первичный ключ:** (id_prizivnik, id_comissar)

#### 6. `prizivnik_meropriyatie` (Связь призывник-мероприятие)

Связующая таблица для связи многие-ко-многим между призывниками и мероприятиями.

**Поля:**
- `id_prizivnik` (integer, NOT NULL, PRIMARY KEY, FOREIGN KEY -> prizivnik) - Ссылка на призывника
- `id_meropriyatiya` (integer, NOT NULL, PRIMARY KEY, FOREIGN KEY -> prizivnoe_meropriyatie) - Ссылка на мероприятие

**Составной первичный ключ:** (id_prizivnik, id_meropriyatiya)

#### 7. `prizivnoe_meropriyatie` (Призывные мероприятия)

Хранит информацию о призывных мероприятиях.

**Поля:**
- `id_meropriyatiya` (integer, PRIMARY KEY, AUTO_INCREMENT) - Уникальный идентификатор мероприятия
- `tip_meropriyatiya` (varchar(100), NOT NULL) - Тип мероприятия
- `data_provedeniya` (timestamp, NOT NULL) - Дата и время проведения
- `mesto_provedeniya` (text) - Место проведения
- `fio_comissara` (varchar(255)) - ФИО комиссара (дублирование данных)
- `id_comissar` (integer, NOT NULL, FOREIGN KEY -> comissar) - Ссылка на комиссара

**Связанные таблицы:**
- `comissar` - связь многие-к-одному
- `prizivnik_meropriyatie` - связь один-ко-многим

#### 8. `voenno_uchetnaya_karta` (Военно-учётные карты)

Хранит информацию о военно-учётных картах призывников.

**Поля:**
- `id_karty` (integer, PRIMARY KEY, AUTO_INCREMENT) - Уникальный идентификатор карты
- `nomer_karty` (varchar(50), NOT NULL) - Номер карты
- `data_postanovki_na_uchet` (date, NOT NULL) - Дата постановки на учёт
- `istoriya_otsrochek` (text) - История отсрочек
- `voenno_uchetnaya_specialnost` (varchar(100)) - Военно-учётная специальность
- `id_prizivnika` (integer, NOT NULL, FOREIGN KEY -> prizivnik) - Ссылка на призывника

**Связанные таблицы:**
- `prizivnik` - связь один-к-одному

#### 9. `voennyi_bilet` (Военные билеты)

Хранит информацию о военных билетах призывников.

**Поля:**
- `id_bileta` (integer, PRIMARY KEY, AUTO_INCREMENT) - Уникальный идентификатор билета
- `nomer_bileta` (varchar(50), NOT NULL) - Номер билета
- `data_vydachi` (date) - Дата выдачи
- `voinskoe_zvanie` (varchar(50)) - Воинское звание
- `kategoria` (varchar(50)) - Категория (дублирование данных из kategoria_godnosti)
- `id_prizivnika` (integer, NOT NULL, FOREIGN KEY -> prizivnik) - Ссылка на призывника
- `id_kategorii` (integer, FOREIGN KEY -> kategoria_godnosti) - Ссылка на категорию годности

**Связанные таблицы:**
- `prizivnik` - связь один-к-одному
- `kategoria_godnosti` - связь многие-к-одному

## Связи между таблицами (Foreign Keys)

1. **voennyi_bilet → prizivnik**
   - `id_prizivnika` → `prizivnik.id_prizivnik` (ON DELETE CASCADE)

2. **voenno_uchetnaya_karta → prizivnik**
   - `id_prizivnika` → `prizivnik.id_prizivnik` (ON DELETE CASCADE)

3. **med_osvidetelstvovanie → kategoria_godnosti**
   - `id_kategorii` → `kategoria_godnosti.id_kategorii` (ON DELETE RESTRICT)

4. **med_osvidetelstvovanie → prizivnik**
   - `id_prizivnika` → `prizivnik.id_prizivnik` (ON DELETE CASCADE)

5. **prizivnoe_meropriyatie → comissar**
   - `id_comissar` → `comissar.id_comissar` (ON DELETE RESTRICT)

6. **prizivnik → voennyi_bilet**
   - `id_voennogo_bileta` → `voennyi_bilet.id_bileta` (ON DELETE SET NULL)

7. **prizivnik_comissar → comissar**
   - `id_comissar` → `comissar.id_comissar` (ON DELETE CASCADE)

8. **prizivnik_comissar → prizivnik**
   - `id_prizivnik` → `prizivnik.id_prizivnik` (ON DELETE CASCADE)

9. **prizivnik → voenno_uchetnaya_karta**
   - `id_voenno_uchetnoi_karty` → `voenno_uchetnaya_karta.id_karty` (ON DELETE SET NULL)

10. **prizivnik_meropriyatie → prizivnoe_meropriyatie**
    - `id_meropriyatiya` → `prizivnoe_meropriyatie.id_meropriyatiya` (ON DELETE CASCADE)

11. **prizivnik_meropriyatie → prizivnik**
    - `id_prizivnik` → `prizivnik.id_prizivnik` (ON DELETE CASCADE)

12. **voennyi_bilet → kategoria_godnosti**
    - `id_kategorii` → `kategoria_godnosti.id_kategorii` (без каскадного удаления)

## Sequences (Последовательности)

Все таблицы используют sequences для автоматической генерации первичных ключей:

- `comissar_id_comissar_seq`
- `kategoria_godnosti_id_kategorii_seq`
- `med_osvidetelstvovanie_id_osvidetelstvovania_seq`
- `prizivnik_id_prizivnik_seq`
- `prizivnoe_meropriyatie_id_meropriyatiya_seq`
- `voenno_uchetnaya_karta_id_karty_seq`
- `voennyi_bilet_id_bileta_seq`

## Примечания

1. В таблице `prizivnoe_meropriyatie` есть поле `fio_comissara`, которое дублирует данные из таблицы `comissar`. Это нарушение нормализации, но может быть оставлено для удобства.

2. В таблице `voennyi_bilet` есть поле `kategoria`, которое дублирует данные из `kategoria_godnosti.nazvanie_kategorii`. Это также нарушение нормализации.

3. Все связи с `prizivnik` используют каскадное удаление (CASCADE), кроме связей с `comissar` и `kategoria_godnosti`, которые используют RESTRICT для предотвращения удаления связанных записей.

4. Связи `prizivnik → voennyi_bilet` и `prizivnik → voenno_uchetnaya_karta` используют SET NULL, что позволяет удалять билеты и карты без удаления призывника.

