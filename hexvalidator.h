#ifndef HEXVALIDATOR_H
#define HEXVALIDATOR_H

#include <QValidator>

class HexValidator : public QValidator {
    Q_OBJECT
public:
    HexValidator(unsigned int digits = 8, QObject *parent = nullptr) :
        QValidator(parent), m_digits(digits) {

    }
    virtual ~HexValidator() {}

    virtual State validate(QString &s, int &pos) const override {
        if (s.isEmpty()) {
            pos = 0;
            s = "0";
            return State::Acceptable;
        }

        if (s.size() > 8) {
            s.resize(8);
            pos = 8;
        }

        for (auto it = s.begin(); it != s.end(); it++) {
            auto c = *it;
            if ((c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'F')) {
                continue;
            } else if (c >= 'a' && c <= 'f') {
                *it = c.toUpper();
            } else {
                return State::Invalid;
            }
        }

        return State::Acceptable;
    }

    virtual void fixup(QString &s) const override {
        erase_if(s, [](auto c) -> bool {
            return !(
                (c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'F') ||
                (c >= 'a' && c <= 'f')
            );
        });
    }

private:
    unsigned int m_digits;
};

#endif // HEXVALIDATOR_H
