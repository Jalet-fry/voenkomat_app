#ifndef DBCONSTANTS_H
#define DBCONSTANTS_H

#include <QString>

namespace Db {
    namespace Tables {
        const QString CALLUP_EVENTS = "prizivnoe_meropriyatie";
        const QString COMMISSIONERS = "comissar";
        const QString CONSCRIPTS = "prizivnik";
        const QString CONSCRIPTS_COMMISSIONERS = "prizivnik_comissar";
        const QString FITNESS_CATEGORIES = "kategoria_godnosti";
        const QString MEDICAL_EXAMINATIONS = "med_osvidetelstvovanie";
        const QString MILITARY_ID_CARDS = "voennyi_bilet";
        const QString SERVICE_RECORD_CARDS = "voenno_uchetnaya_karta";
    }

    namespace Conscripts {
        const QString CONSCRIPT_ID = "id_prizivnik";
        const QString FULL_NAME = "fio";
        const QString BIRTH_DATE = "data_rozhdeniya";
        const QString RESIDENCE_ADDRESS = "adres_prozhivaniya";
        const QString PASSPORT_NUMBER = "nomer_pasporta";
        const QString MILITARY_TICKET_ID = "id_voennogo_bileta";
        const QString REGISTRATION_CARD_ID = "id_voenno_uchetnoi_karty";
    }

    namespace MilitaryIdCards {
        const QString TICKET_ID = "id_bileta";
        const QString TICKET_NUMBER = "nomer_bileta";
        const QString CONSCRIPT_ID = "id_prizivnika";
        const QString CATEGORY_ID = "id_kategorii";
    }
}

#endif // DBCONSTANTS_H
