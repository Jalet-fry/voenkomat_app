// TODO: [REVIEW] OK. Matches create_schema.sql.
#ifndef DBCONSTANTS_H
#define DBCONSTANTS_H

#include <QString>

namespace Db {
    namespace Tables {
        const QString CALLUP_EVENTS = "callup_events";
        const QString COMMISSIONERS = "commissioners";
        const QString CONSCRIPTS = "conscripts";
        const QString CONSCRIPTS_COMMISSIONERS = "conscripts_commissioners";
        const QString CONSCRIPTS_EVENTS = "conscripts_events";
        const QString FITNESS_CATEGORIES = "fitness_categories";
        const QString MEDICAL_EXAMINATIONS = "medical_examinations";
        const QString MILITARY_ID_CARDS = "military_id_cards";
        const QString SERVICE_RECORD_CARDS = "service_record_cards";
        const QString TEST_TABLE = "test_table";
    }

    namespace CallupEvents {
        const QString EVENT_ID = "event_id";
        const QString EVENT_TYPE = "event_type";
        const QString EVENT_DATETIME = "event_datetime";
        const QString EVENT_LOCATION = "event_location";
        const QString COMMISSIONER_FULL_NAME = "commissioner_full_name";
        const QString COMMISSIONER_ID = "commissioner_id";
    }

    namespace Commissioners {
        const QString COMMISSIONER_ID = "commissioner_id";
        const QString FULL_NAME = "full_name";
        const QString POSITION = "position";
        const QString YEARS_OF_SERVICE = "years_of_service";
        const QString PHONE_NUMBER = "phone_number";
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

    namespace ConscriptsCommissioners {
        const QString CONSCRIPT_ID = "conscript_id";
        const QString COMMISSIONER_ID = "commissioner_id";
        const QString INTERACTION_DATE = "interaction_date";
        const QString OFFICE_NUMBER = "office_number";
    }

    namespace FitnessCategories {
        const QString CATEGORY_ID = "category_id";
        const QString CATEGORY_NAME = "category_name";
        const QString RESTRICTION_DESCRIPTION = "restriction_description";
        const QString CATEGORY_INDEX = "category_index";
        const QString CATEGORY_BASIS = "category_basis";
    }

    namespace MedicalExaminations {
        const QString CERTIFICATION_ID = "certification_id";
        const QString EXAMINATION_DATE = "examination_date";
        const QString EXAMINATION_RESULTS = "examination_results";
        const QString DOCTOR_FULL_NAME = "doctor_full_name";
        const QString CONCLUSION = "conclusion";
        const QString CONSCRIPT_ID = "conscript_id";
        const QString CATEGORY_ID = "category_id";
    }

    namespace MilitaryIdCards {
        const QString TICKET_ID = "ticket_id";
        const QString TICKET_NUMBER = "ticket_number";
        const QString ISSUE_DATE = "issue_date";
        const QString MILITARY_RANK = "military_rank";
        const QString CATEGORY = "category";
        const QString CONSCRIPT_ID = "conscript_id";
        const QString CATEGORY_ID = "category_id";
    }

    namespace ServiceRecordCards {
        const QString CARD_ID = "card_id";
        const QString CARD_NUMBER = "card_number";
        const QString REGISTRATION_DATE = "registration_date";
        const QString DEFERMENT_HISTORY = "deferment_history";
        const QString MILITARY_SPECIALTY = "military_specialty";
        const QString CONSCRIPT_ID = "conscript_id";
    }
}

#endif // DBCONSTANTS_H
