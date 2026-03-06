#ifndef DBCONSTANTS_H
#define DBCONSTANTS_H

#include <QString>

/**
 * @brief TODO: [LAB1] Имена таблиц и полей приведены в полное соответствие с реляционной схемой (create_schema.sql).
 * Это критически важно для корректной работы SQL запросов и соответствия ТЗ.
 */
namespace Db {
    namespace Tables {
        const QString CALLUP_EVENTS = "callup_events";
        const QString COMMISSIONERS = "commissioners";
        const QString CONSCRIPTS = "conscripts";
        const QString CONSCRIPTS_COMMISSIONERS = "conscripts_commissioners";
        const QString FITNESS_CATEGORIES = "fitness_categories";
        const QString MEDICAL_EXAMINATIONS = "medical_examinations";
        const QString MILITARY_ID_CARDS = "military_id_cards";
        const QString SERVICE_RECORD_CARDS = "service_record_cards";
    }

    namespace Conscripts {
        const QString CONSCRIPT_ID = "conscript_id";
        const QString FULL_NAME = "full_name";
        const QString BIRTH_DATE = "birth_date";
        const QString RESIDENCE_ADDRESS = "residence_address";
        const QString PASSPORT_NUMBER = "passport_number";
        const QString MILITARY_TICKET_ID = "military_ticket_id";
        const QString REGISTRATION_CARD_ID = "registration_card_id";
    }

    namespace MilitaryIdCards {
        const QString TICKET_ID = "ticket_id";
        const QString TICKET_NUMBER = "ticket_number";
        const QString CONSCRIPT_ID = "conscript_id";
        const QString CATEGORY_ID = "category_id";
    }
}

#endif // DBCONSTANTS_H
