--
-- PostgreSQL database dump
--

\restrict 1CiiX0RCBONHTZB63lXQOiPZTEmiDLIrygI9ttFd7HfiXpQYZhdONMRVt9h2D1J

-- Dumped from database version 17.6
-- Dumped by pg_dump version 17.6

-- Started on 2025-11-03 16:43:14

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 222 (class 1259 OID 17121)
-- Name: comissar; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.comissar (
    id_comissar integer NOT NULL,
    fio character varying(255) NOT NULL,
    dolzhnost character varying(100) NOT NULL,
    stazh_raboty integer,
    kontaktnyi_telefon character varying(20)
);


ALTER TABLE public.comissar OWNER TO postgres;

--
-- TOC entry 221 (class 1259 OID 17120)
-- Name: comissar_id_comissar_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.comissar_id_comissar_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.comissar_id_comissar_seq OWNER TO postgres;

--
-- TOC entry 5006 (class 0 OID 0)
-- Dependencies: 221
-- Name: comissar_id_comissar_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.comissar_id_comissar_seq OWNED BY public.comissar.id_comissar;


--
-- TOC entry 220 (class 1259 OID 17111)
-- Name: kategoria_godnosti; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.kategoria_godnosti (
    id_kategorii integer NOT NULL,
    nazvanie_kategorii character varying(1) NOT NULL,
    opisanie_ogranichenii text,
    index_kategorii integer,
    osnovanie_dlya_kategorii text
);


ALTER TABLE public.kategoria_godnosti OWNER TO postgres;

--
-- TOC entry 219 (class 1259 OID 17110)
-- Name: kategoria_godnosti_id_kategorii_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.kategoria_godnosti_id_kategorii_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.kategoria_godnosti_id_kategorii_seq OWNER TO postgres;

--
-- TOC entry 5007 (class 0 OID 0)
-- Dependencies: 219
-- Name: kategoria_godnosti_id_kategorii_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.kategoria_godnosti_id_kategorii_seq OWNED BY public.kategoria_godnosti.id_kategorii;


--
-- TOC entry 230 (class 1259 OID 17167)
-- Name: med_osvidetelstvovanie; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.med_osvidetelstvovanie (
    id_osvidetelstvovania integer NOT NULL,
    data_provedeniya date NOT NULL,
    rezultaty_obsledovania text,
    fio_vracha character varying(255) NOT NULL,
    zaklyuchenie text,
    id_prizivnika integer NOT NULL,
    id_kategorii integer NOT NULL
);


ALTER TABLE public.med_osvidetelstvovanie OWNER TO postgres;

--
-- TOC entry 229 (class 1259 OID 17166)
-- Name: med_osvidetelstvovanie_id_osvidetelstvovania_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.med_osvidetelstvovanie_id_osvidetelstvovania_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.med_osvidetelstvovanie_id_osvidetelstvovania_seq OWNER TO postgres;

--
-- TOC entry 5008 (class 0 OID 0)
-- Dependencies: 229
-- Name: med_osvidetelstvovanie_id_osvidetelstvovania_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.med_osvidetelstvovanie_id_osvidetelstvovania_seq OWNED BY public.med_osvidetelstvovanie.id_osvidetelstvovania;


--
-- TOC entry 224 (class 1259 OID 17128)
-- Name: prizivnik; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.prizivnik (
    id_prizivnik integer NOT NULL,
    fio character varying(255) NOT NULL,
    data_rozhdeniya date NOT NULL,
    adres_prozhivaniya text,
    nomer_pasporta character varying(20) NOT NULL,
    id_voennogo_bileta integer,
    id_voenno_uchetnoi_karty integer
);


ALTER TABLE public.prizivnik OWNER TO postgres;

--
-- TOC entry 233 (class 1259 OID 17184)
-- Name: prizivnik_comissar; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.prizivnik_comissar (
    id_prizivnik integer NOT NULL,
    id_comissar integer NOT NULL,
    data_vzaimodeistviya date,
    nomer_kabineta character varying(10)
);


ALTER TABLE public.prizivnik_comissar OWNER TO postgres;

--
-- TOC entry 223 (class 1259 OID 17127)
-- Name: prizivnik_id_prizivnik_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.prizivnik_id_prizivnik_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.prizivnik_id_prizivnik_seq OWNER TO postgres;

--
-- TOC entry 5009 (class 0 OID 0)
-- Dependencies: 223
-- Name: prizivnik_id_prizivnik_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.prizivnik_id_prizivnik_seq OWNED BY public.prizivnik.id_prizivnik;


--
-- TOC entry 234 (class 1259 OID 17189)
-- Name: prizivnik_meropriyatie; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.prizivnik_meropriyatie (
    id_prizivnik integer NOT NULL,
    id_meropriyatiya integer NOT NULL
);


ALTER TABLE public.prizivnik_meropriyatie OWNER TO postgres;

--
-- TOC entry 232 (class 1259 OID 17176)
-- Name: prizivnoe_meropriyatie; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.prizivnoe_meropriyatie (
    id_meropriyatiya integer NOT NULL,
    tip_meropriyatiya character varying(100) NOT NULL,
    data_provedeniya timestamp without time zone NOT NULL,
    mesto_provedeniya text,
    fio_comissara character varying(255),
    id_comissar integer NOT NULL
);


ALTER TABLE public.prizivnoe_meropriyatie OWNER TO postgres;

--
-- TOC entry 231 (class 1259 OID 17175)
-- Name: prizivnoe_meropriyatie_id_meropriyatiya_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.prizivnoe_meropriyatie_id_meropriyatiya_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.prizivnoe_meropriyatie_id_meropriyatiya_seq OWNER TO postgres;

--
-- TOC entry 5010 (class 0 OID 0)
-- Dependencies: 231
-- Name: prizivnoe_meropriyatie_id_meropriyatiya_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.prizivnoe_meropriyatie_id_meropriyatiya_seq OWNED BY public.prizivnoe_meropriyatie.id_meropriyatiya;


--
-- TOC entry 226 (class 1259 OID 17143)
-- Name: voenno_uchetnaya_karta; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.voenno_uchetnaya_karta (
    id_karty integer NOT NULL,
    nomer_karty character varying(50) NOT NULL,
    data_postanovki_na_uchet date NOT NULL,
    istoriya_otsrochek text,
    voenno_uchetnaya_specialnost character varying(100),
    id_prizivnika integer NOT NULL
);


ALTER TABLE public.voenno_uchetnaya_karta OWNER TO postgres;

--
-- TOC entry 225 (class 1259 OID 17142)
-- Name: voenno_uchetnaya_karta_id_karty_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.voenno_uchetnaya_karta_id_karty_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.voenno_uchetnaya_karta_id_karty_seq OWNER TO postgres;

--
-- TOC entry 5011 (class 0 OID 0)
-- Dependencies: 225
-- Name: voenno_uchetnaya_karta_id_karty_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.voenno_uchetnaya_karta_id_karty_seq OWNED BY public.voenno_uchetnaya_karta.id_karty;


--
-- TOC entry 228 (class 1259 OID 17156)
-- Name: voennyi_bilet; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.voennyi_bilet (
    id_bileta integer NOT NULL,
    nomer_bileta character varying(50) NOT NULL,
    data_vydachi date,
    voinskoe_zvanie character varying(50),
    kategoria character varying(50),
    id_prizivnika integer NOT NULL,
    id_kategorii integer
);


ALTER TABLE public.voennyi_bilet OWNER TO postgres;

--
-- TOC entry 227 (class 1259 OID 17155)
-- Name: voennyi_bilet_id_bileta_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.voennyi_bilet_id_bileta_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.voennyi_bilet_id_bileta_seq OWNER TO postgres;

--
-- TOC entry 5012 (class 0 OID 0)
-- Dependencies: 227
-- Name: voennyi_bilet_id_bileta_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.voennyi_bilet_id_bileta_seq OWNED BY public.voennyi_bilet.id_bileta;


--
-- TOC entry 4783 (class 2604 OID 17124)
-- Name: comissar id_comissar; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.comissar ALTER COLUMN id_comissar SET DEFAULT nextval('public.comissar_id_comissar_seq'::regclass);


--
-- TOC entry 4782 (class 2604 OID 17114)
-- Name: kategoria_godnosti id_kategorii; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.kategoria_godnosti ALTER COLUMN id_kategorii SET DEFAULT nextval('public.kategoria_godnosti_id_kategorii_seq'::regclass);


--
-- TOC entry 4787 (class 2604 OID 17170)
-- Name: med_osvidetelstvovanie id_osvidetelstvovania; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.med_osvidetelstvovanie ALTER COLUMN id_osvidetelstvovania SET DEFAULT nextval('public.med_osvidetelstvovanie_id_osvidetelstvovania_seq'::regclass);


--
-- TOC entry 4784 (class 2604 OID 17131)
-- Name: prizivnik id_prizivnik; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik ALTER COLUMN id_prizivnik SET DEFAULT nextval('public.prizivnik_id_prizivnik_seq'::regclass);


--
-- TOC entry 4788 (class 2604 OID 17179)
-- Name: prizivnoe_meropriyatie id_meropriyatiya; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnoe_meropriyatie ALTER COLUMN id_meropriyatiya SET DEFAULT nextval('public.prizivnoe_meropriyatie_id_meropriyatiya_seq'::regclass);


--
-- TOC entry 4785 (class 2604 OID 17146)
-- Name: voenno_uchetnaya_karta id_karty; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voenno_uchetnaya_karta ALTER COLUMN id_karty SET DEFAULT nextval('public.voenno_uchetnaya_karta_id_karty_seq'::regclass);


--
-- TOC entry 4786 (class 2604 OID 17159)
-- Name: voennyi_bilet id_bileta; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voennyi_bilet ALTER COLUMN id_bileta SET DEFAULT nextval('public.voennyi_bilet_id_bileta_seq'::regclass);


--
-- TOC entry 4988 (class 0 OID 17121)
-- Dependencies: 222
-- Data for Name: comissar; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.comissar (id_comissar, fio, dolzhnost, stazh_raboty, kontaktnyi_telefon) FROM stdin;
1	Иванов Иван Иванович	Военный комиссар	15	+375291111111
2	Петров Пётр Петрович	Заместитель комиссара	12	+375292222222
3	Сидорова Анна Сергеевна	Начальник отдела	8	+375293333333
4	Кузнецов Олег Викторович	Старший офицер	20	+375294444444
5	Соколова Мария Игоревна	Офицер по работе	6	+375295555555
6	Орлова Дарья Петровна	Специалист по учёту	10	+375296666666
7	Смирнов Даниил Ильич	Медицинский эксперт	14	+375297777777
8	Егорова Лидия Павловна	Психолог	7	+375298888888
9	Мороз Артём Андреевич	Юрист	11	+375299999999
10	Фёдоров Никита Романович	Инспектор	9	+375291010101
\.


--
-- TOC entry 4986 (class 0 OID 17111)
-- Dependencies: 220
-- Data for Name: kategoria_godnosti; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.kategoria_godnosti (id_kategorii, nazvanie_kategorii, opisanie_ogranichenii, index_kategorii, osnovanie_dlya_kategorii) FROM stdin;
1	А	Годен к военной службе без ограничений	1	Отличное здоровье, все системы в норме
2	А	Годен к военной службе с незначительными ограничениями	2	Незначительные отклонения, не влияющие на службу
3	А	Годен к военной службе с ограничениями по роду войск	3	Ограничения по выбору воинской специальности
4	Б	Годен с ограничениями по физической нагрузке	1	Ограничения по интенсивности физических нагрузок
5	Б	Годен с ограничениями по климатическим условиям	2	Ограничения по службе в определенных климатических зонах
6	Б	Годен с ограничениями по условиям службы	3	Ограничения по условиям несения службы
7	В	Ограниченно годен в мирное время	1	Ограниченная годность в мирное время
8	В	Ограниченно годен в военное время	2	Ограниченная годность в военное время
9	Г	Временно не годен (до 6 месяцев)	1	Временные проблемы со здоровьем
10	Г	Временно не годен (до 12 месяцев)	2	Длительные временные проблемы
11	Д	Не годен к военной службе	1	Стойкие проблемы со здоровьем
\.


--
-- TOC entry 4996 (class 0 OID 17167)
-- Dependencies: 230
-- Data for Name: med_osvidetelstvovanie; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.med_osvidetelstvovanie (id_osvidetelstvovania, data_provedeniya, rezultaty_obsledovania, fio_vracha, zaklyuchenie, id_prizivnika, id_kategorii) FROM stdin;
1	2023-01-10	Полное обследование пройдено	Смирнов А.А.	Годен к военной службе	1	1
2	2023-02-15	Обнаружены незначительные отклонения	Петров В.В.	Годен с ограничениями	2	4
3	2023-03-05	Обследование без патологий	Иванова С.С.	Годен к военной службе	3	1
4	2023-04-12	Выявлены серьёзные проблемы	Козлов Д.Д.	Ограниченно годен	4	7
5	2023-05-20	Незначительные отклонения	Соколова Е.Е.	Годен с ограничениями	5	4
6	2023-06-08	Обследование пройдено успешно	Морозов Ф.Ф.	Годен к военной службе	6	1
7	2023-07-14	Серьёзные проблемы со здоровьем	Волков Г.Г.	Ограниченно годен	7	7
8	2023-08-25	Незначительные ограничения	Лебедев И.И.	Годен с ограничениями	8	4
9	2023-09-03	Полное здоровье	Новиков К.К.	Годен к военной службе	9	1
10	2023-10-11	Умеренные ограничения	Фёдоров Л.Л.	Годен с ограничениями	10	4
\.


--
-- TOC entry 4990 (class 0 OID 17128)
-- Dependencies: 224
-- Data for Name: prizivnik; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.prizivnik (id_prizivnik, fio, data_rozhdeniya, adres_prozhivaniya, nomer_pasporta, id_voennogo_bileta, id_voenno_uchetnoi_karty) FROM stdin;
1	Александров Алексей Сергеевич	2000-03-15	г. Минск, ул. Ленина, 15	MP1234567	1	1
2	Борисов Борис Борисович	1999-07-22	г. Гомель, ул. Советская, 8	MP2345678	2	2
3	Васильев Василий Васильевич	2001-11-08	г. Витебск, ул. Пушкина, 25	MP3456789	3	3
4	Григорьев Григорий Григорьевич	2000-05-14	г. Могилёв, ул. Мира, 42	MP4567890	4	4
5	Дмитриев Дмитрий Дмитриевич	1999-12-03	г. Брест, ул. Гагарина, 17	MP5678901	5	5
6	Егоров Егор Егорович	2001-09-19	г. Гродно, ул. Комсомольская, 33	MP6789012	6	6
7	Жуков Жан Жанович	2000-01-27	г. Минск, ул. Независимости, 95	MP7890123	7	7
8	Зайцев Захар Захарович	1999-08-11	г. Могилёв, ул. Ленина, 72	MP8901234	8	8
9	Иванов Игорь Игоревич	2001-04-06	г. Витебск, ул. Мира, 58	MP9012345	9	9
10	Козлов Константин Константинович	2000-10-30	г. Гомель, ул. Советская, 91	MP0123456	10	10
\.


--
-- TOC entry 4999 (class 0 OID 17184)
-- Dependencies: 233
-- Data for Name: prizivnik_comissar; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.prizivnik_comissar (id_prizivnik, id_comissar, data_vzaimodeistviya, nomer_kabineta) FROM stdin;
1	1	2023-01-15	101
2	2	2023-02-20	102
3	3	2023-03-10	103
4	4	2023-04-05	104
5	5	2023-05-12	105
6	6	2023-06-18	106
7	7	2023-07-25	107
8	8	2023-08-30	108
9	9	2023-09-14	109
10	10	2023-10-22	110
\.


--
-- TOC entry 5000 (class 0 OID 17189)
-- Dependencies: 234
-- Data for Name: prizivnik_meropriyatie; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.prizivnik_meropriyatie (id_prizivnik, id_meropriyatiya) FROM stdin;
1	1
2	2
3	3
4	4
5	5
6	6
7	7
8	8
9	9
10	10
\.


--
-- TOC entry 4998 (class 0 OID 17176)
-- Dependencies: 232
-- Data for Name: prizivnoe_meropriyatie; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.prizivnoe_meropriyatie (id_meropriyatiya, tip_meropriyatiya, data_provedeniya, mesto_provedeniya, fio_comissara, id_comissar) FROM stdin;
1	Явка в военкомат	2023-01-15 09:00:00	Военкомат Центрального района	Иванов И.И.	1
2	Медицинское освидетельствование	2023-02-20 10:00:00	Военный госпиталь	Петров П.П.	2
3	Психологическое тестирование	2023-03-10 11:00:00	Центр психологической помощи	Сидорова А.С.	3
4	Совещание призывной комиссии	2023-04-05 14:00:00	Зал заседаний военкомата	Кузнецов О.В.	4
5	Отправка в воинскую часть	2023-05-12 08:00:00	Железнодорожный вокзал	Соколова М.И.	5
6	Повторная явка	2023-06-18 09:30:00	Военкомат Советского района	Орлова Д.П.	6
7	Дополнительное обследование	2023-07-25 10:30:00	Специализированная клиника	Смирнов Д.И.	7
8	Апелляция по категории	2023-08-30 15:00:00	Областной военкомат	Егорова Л.П.	8
9	Окончательное решение	2023-09-14 16:00:00	Зал заседаний	Мороз А.А.	9
10	Отправка на службу	2023-10-22 07:00:00	Сборный пункт	Фёдоров Н.Р.	10
\.


--
-- TOC entry 4992 (class 0 OID 17143)
-- Dependencies: 226
-- Data for Name: voenno_uchetnaya_karta; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.voenno_uchetnaya_karta (id_karty, nomer_karty, data_postanovki_na_uchet, istoriya_otsrochek, voenno_uchetnaya_specialnost, id_prizivnika) FROM stdin;
1	ВУК-001	2018-01-15	Отсрочка по учёбе 2018-2022	Специалист по связи	1
2	ВУК-002	2017-09-10	Отсрочка по семейным обстоятельствам 2019-2021	Водитель	2
3	ВУК-003	2019-03-22	Без отсрочек	Радист	3
4	ВУК-004	2018-11-05	Отсрочка по здоровью 2020-2021	Снайпер	4
5	ВУК-005	2017-06-18	Отсрочка по учёбе 2018-2023	Инженер	5
6	ВУК-006	2019-08-14	Без отсрочек	Медик	6
7	ВУК-007	2018-04-03	Отсрочка по работе 2020-2022	Переводчик	7
8	ВУК-008	2017-12-28	Отсрочка по семейным обстоятельствам 2019-2020	Повар	8
9	ВУК-009	2019-07-11	Без отсрочек	Связист	9
10	ВУК-010	2018-02-16	Отсрочка по учёбе 2019-2024	Программист	10
\.


--
-- TOC entry 4994 (class 0 OID 17156)
-- Dependencies: 228
-- Data for Name: voennyi_bilet; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.voennyi_bilet (id_bileta, nomer_bileta, data_vydachi, voinskoe_zvanie, kategoria, id_prizivnika, id_kategorii) FROM stdin;
9	ВБ-009	2023-09-14	Младший лейтенант	А	9	1
6	ВБ-006	2023-06-18	Старшина	А	6	1
3	ВБ-003	2023-03-10	Младший сержант	А	3	1
1	ВБ-001	2023-01-15	Рядовой	А	1	1
10	ВБ-010	2023-10-22	Лейтенант	Б	10	4
8	ВБ-008	2023-08-30	Старший прапорщик	Б	8	4
5	ВБ-005	2023-05-12	Старший сержант	Б	5	4
2	ВБ-002	2023-02-20	Ефрейтор	Б	2	4
7	ВБ-007	2023-07-25	Прапорщик	В	7	7
4	ВБ-004	2023-04-05	Сержант	В	4	7
\.


--
-- TOC entry 5013 (class 0 OID 0)
-- Dependencies: 221
-- Name: comissar_id_comissar_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.comissar_id_comissar_seq', 10, true);


--
-- TOC entry 5014 (class 0 OID 0)
-- Dependencies: 219
-- Name: kategoria_godnosti_id_kategorii_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.kategoria_godnosti_id_kategorii_seq', 5, true);


--
-- TOC entry 5015 (class 0 OID 0)
-- Dependencies: 229
-- Name: med_osvidetelstvovanie_id_osvidetelstvovania_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.med_osvidetelstvovanie_id_osvidetelstvovania_seq', 10, true);


--
-- TOC entry 5016 (class 0 OID 0)
-- Dependencies: 223
-- Name: prizivnik_id_prizivnik_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.prizivnik_id_prizivnik_seq', 10, true);


--
-- TOC entry 5017 (class 0 OID 0)
-- Dependencies: 231
-- Name: prizivnoe_meropriyatie_id_meropriyatiya_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.prizivnoe_meropriyatie_id_meropriyatiya_seq', 10, true);


--
-- TOC entry 5018 (class 0 OID 0)
-- Dependencies: 225
-- Name: voenno_uchetnaya_karta_id_karty_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.voenno_uchetnaya_karta_id_karty_seq', 10, true);


--
-- TOC entry 5019 (class 0 OID 0)
-- Dependencies: 227
-- Name: voennyi_bilet_id_bileta_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.voennyi_bilet_id_bileta_seq', 10, true);


--
-- TOC entry 4794 (class 2606 OID 17126)
-- Name: comissar comissar_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.comissar
    ADD CONSTRAINT comissar_pkey PRIMARY KEY (id_comissar);


--
-- TOC entry 4790 (class 2606 OID 17303)
-- Name: kategoria_godnosti kategoria_godnosti_nazvanie_index_unique; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.kategoria_godnosti
    ADD CONSTRAINT kategoria_godnosti_nazvanie_index_unique UNIQUE (nazvanie_kategorii, index_kategorii);


--
-- TOC entry 4792 (class 2606 OID 17119)
-- Name: kategoria_godnosti kategoria_godnosti_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.kategoria_godnosti
    ADD CONSTRAINT kategoria_godnosti_pkey PRIMARY KEY (id_kategorii);


--
-- TOC entry 4818 (class 2606 OID 17174)
-- Name: med_osvidetelstvovanie med_osvidetelstvovanie_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.med_osvidetelstvovanie
    ADD CONSTRAINT med_osvidetelstvovanie_pkey PRIMARY KEY (id_osvidetelstvovania);


--
-- TOC entry 4824 (class 2606 OID 17188)
-- Name: prizivnik_comissar prizivnik_comissar_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik_comissar
    ADD CONSTRAINT prizivnik_comissar_pkey PRIMARY KEY (id_prizivnik, id_comissar);


--
-- TOC entry 4796 (class 2606 OID 17141)
-- Name: prizivnik prizivnik_id_voenno_uchetnoi_karty_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik
    ADD CONSTRAINT prizivnik_id_voenno_uchetnoi_karty_key UNIQUE (id_voenno_uchetnoi_karty);


--
-- TOC entry 4798 (class 2606 OID 17139)
-- Name: prizivnik prizivnik_id_voennogo_bileta_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik
    ADD CONSTRAINT prizivnik_id_voennogo_bileta_key UNIQUE (id_voennogo_bileta);


--
-- TOC entry 4827 (class 2606 OID 17193)
-- Name: prizivnik_meropriyatie prizivnik_meropriyatie_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik_meropriyatie
    ADD CONSTRAINT prizivnik_meropriyatie_pkey PRIMARY KEY (id_prizivnik, id_meropriyatiya);


--
-- TOC entry 4800 (class 2606 OID 17137)
-- Name: prizivnik prizivnik_nomer_pasporta_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik
    ADD CONSTRAINT prizivnik_nomer_pasporta_key UNIQUE (nomer_pasporta);


--
-- TOC entry 4802 (class 2606 OID 17135)
-- Name: prizivnik prizivnik_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik
    ADD CONSTRAINT prizivnik_pkey PRIMARY KEY (id_prizivnik);


--
-- TOC entry 4821 (class 2606 OID 17183)
-- Name: prizivnoe_meropriyatie prizivnoe_meropriyatie_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnoe_meropriyatie
    ADD CONSTRAINT prizivnoe_meropriyatie_pkey PRIMARY KEY (id_meropriyatiya);


--
-- TOC entry 4804 (class 2606 OID 17154)
-- Name: voenno_uchetnaya_karta voenno_uchetnaya_karta_id_prizivnika_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voenno_uchetnaya_karta
    ADD CONSTRAINT voenno_uchetnaya_karta_id_prizivnika_key UNIQUE (id_prizivnika);


--
-- TOC entry 4806 (class 2606 OID 17152)
-- Name: voenno_uchetnaya_karta voenno_uchetnaya_karta_nomer_karty_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voenno_uchetnaya_karta
    ADD CONSTRAINT voenno_uchetnaya_karta_nomer_karty_key UNIQUE (nomer_karty);


--
-- TOC entry 4808 (class 2606 OID 17150)
-- Name: voenno_uchetnaya_karta voenno_uchetnaya_karta_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voenno_uchetnaya_karta
    ADD CONSTRAINT voenno_uchetnaya_karta_pkey PRIMARY KEY (id_karty);


--
-- TOC entry 4810 (class 2606 OID 17165)
-- Name: voennyi_bilet voennyi_bilet_id_prizivnika_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voennyi_bilet
    ADD CONSTRAINT voennyi_bilet_id_prizivnika_key UNIQUE (id_prizivnika);


--
-- TOC entry 4812 (class 2606 OID 17163)
-- Name: voennyi_bilet voennyi_bilet_nomer_bileta_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voennyi_bilet
    ADD CONSTRAINT voennyi_bilet_nomer_bileta_key UNIQUE (nomer_bileta);


--
-- TOC entry 4814 (class 2606 OID 17161)
-- Name: voennyi_bilet voennyi_bilet_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voennyi_bilet
    ADD CONSTRAINT voennyi_bilet_pkey PRIMARY KEY (id_bileta);


--
-- TOC entry 4815 (class 1259 OID 17250)
-- Name: idx_med_kategoria; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX idx_med_kategoria ON public.med_osvidetelstvovanie USING btree (id_kategorii);


--
-- TOC entry 4816 (class 1259 OID 17249)
-- Name: idx_med_prizivnik; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX idx_med_prizivnik ON public.med_osvidetelstvovanie USING btree (id_prizivnika);


--
-- TOC entry 4819 (class 1259 OID 17251)
-- Name: idx_meropriyatie_comissar; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX idx_meropriyatie_comissar ON public.prizivnoe_meropriyatie USING btree (id_comissar);


--
-- TOC entry 4822 (class 1259 OID 17252)
-- Name: idx_prizivnik_comissar_comissar; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX idx_prizivnik_comissar_comissar ON public.prizivnik_comissar USING btree (id_comissar);


--
-- TOC entry 4825 (class 1259 OID 17253)
-- Name: idx_prizivnik_meropriyatie_meropriyatie; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX idx_prizivnik_meropriyatie_meropriyatie ON public.prizivnik_meropriyatie USING btree (id_meropriyatiya);


--
-- TOC entry 4831 (class 2606 OID 17199)
-- Name: voennyi_bilet fk_bilet_prizivnik; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voennyi_bilet
    ADD CONSTRAINT fk_bilet_prizivnik FOREIGN KEY (id_prizivnika) REFERENCES public.prizivnik(id_prizivnik) ON DELETE CASCADE;


--
-- TOC entry 4830 (class 2606 OID 17194)
-- Name: voenno_uchetnaya_karta fk_karta_prizivnik; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voenno_uchetnaya_karta
    ADD CONSTRAINT fk_karta_prizivnik FOREIGN KEY (id_prizivnika) REFERENCES public.prizivnik(id_prizivnik) ON DELETE CASCADE;


--
-- TOC entry 4833 (class 2606 OID 17304)
-- Name: med_osvidetelstvovanie fk_med_kategoria; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.med_osvidetelstvovanie
    ADD CONSTRAINT fk_med_kategoria FOREIGN KEY (id_kategorii) REFERENCES public.kategoria_godnosti(id_kategorii) ON DELETE RESTRICT;


--
-- TOC entry 4834 (class 2606 OID 17214)
-- Name: med_osvidetelstvovanie fk_med_prizivnik; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.med_osvidetelstvovanie
    ADD CONSTRAINT fk_med_prizivnik FOREIGN KEY (id_prizivnika) REFERENCES public.prizivnik(id_prizivnik) ON DELETE CASCADE;


--
-- TOC entry 4835 (class 2606 OID 17224)
-- Name: prizivnoe_meropriyatie fk_meropriyatie_comissar; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnoe_meropriyatie
    ADD CONSTRAINT fk_meropriyatie_comissar FOREIGN KEY (id_comissar) REFERENCES public.comissar(id_comissar) ON DELETE RESTRICT;


--
-- TOC entry 4828 (class 2606 OID 17204)
-- Name: prizivnik fk_prizivnik_bilet; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik
    ADD CONSTRAINT fk_prizivnik_bilet FOREIGN KEY (id_voennogo_bileta) REFERENCES public.voennyi_bilet(id_bileta) ON DELETE SET NULL;


--
-- TOC entry 4836 (class 2606 OID 17234)
-- Name: prizivnik_comissar fk_prizivnik_comissar_comissar; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik_comissar
    ADD CONSTRAINT fk_prizivnik_comissar_comissar FOREIGN KEY (id_comissar) REFERENCES public.comissar(id_comissar) ON DELETE CASCADE;


--
-- TOC entry 4837 (class 2606 OID 17229)
-- Name: prizivnik_comissar fk_prizivnik_comissar_prizivnik; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik_comissar
    ADD CONSTRAINT fk_prizivnik_comissar_prizivnik FOREIGN KEY (id_prizivnik) REFERENCES public.prizivnik(id_prizivnik) ON DELETE CASCADE;


--
-- TOC entry 4829 (class 2606 OID 17209)
-- Name: prizivnik fk_prizivnik_karta; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik
    ADD CONSTRAINT fk_prizivnik_karta FOREIGN KEY (id_voenno_uchetnoi_karty) REFERENCES public.voenno_uchetnaya_karta(id_karty) ON DELETE SET NULL;


--
-- TOC entry 4838 (class 2606 OID 17244)
-- Name: prizivnik_meropriyatie fk_prizivnik_meropriyatie_meropriyatie; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik_meropriyatie
    ADD CONSTRAINT fk_prizivnik_meropriyatie_meropriyatie FOREIGN KEY (id_meropriyatiya) REFERENCES public.prizivnoe_meropriyatie(id_meropriyatiya) ON DELETE CASCADE;


--
-- TOC entry 4839 (class 2606 OID 17239)
-- Name: prizivnik_meropriyatie fk_prizivnik_meropriyatie_prizivnik; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.prizivnik_meropriyatie
    ADD CONSTRAINT fk_prizivnik_meropriyatie_prizivnik FOREIGN KEY (id_prizivnik) REFERENCES public.prizivnik(id_prizivnik) ON DELETE CASCADE;


--
-- TOC entry 4832 (class 2606 OID 17309)
-- Name: voennyi_bilet fk_voennyi_bilet_kategoria; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.voennyi_bilet
    ADD CONSTRAINT fk_voennyi_bilet_kategoria FOREIGN KEY (id_kategorii) REFERENCES public.kategoria_godnosti(id_kategorii);


-- Completed on 2025-11-03 16:43:15

--
-- PostgreSQL database dump complete
--

\unrestrict 1CiiX0RCBONHTZB63lXQOiPZTEmiDLIrygI9ttFd7HfiXpQYZhdONMRVt9h2D1J

