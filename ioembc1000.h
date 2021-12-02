#ifndef IOEMBC1000_H
#define IOEMBC1000_H
#include <stdint.h>

#ifdef IOEMBC1000_QT
#include <QObject>
#endif

namespace embc {
    namespace io {
        uint8_t read(uint8_t target_addr);
        bool write(uint8_t target_addr, uint8_t v);
    }

    namespace gpio {

        enum pin_t {
            GPI0 = (1 << 4),
            GPI1 = (1 << 7),
            GPI2 = (1 << 6),
            GPI3 = (1 << 0),
            GPI4 = (1 << 1),
            GPI5 = (1 << 2),
            GPI6 = (1 << 3),
            GPI7 = (1 << 5),

            GPO0 = (1 << 0),
            GPO1 = (1 << 1),
            GPO2 = (1 << 2),
            GPO3 = (1 << 7),
            GPO4 = (1 << 6),
            GPO5 = (1 << 5),
            GPO6 = (1 << 4),
            GPO7 = (1 << 3)
        };

#ifdef IOEMBC1000_QT
        class IOEMBC1000 : public QObject {
            Q_OBJECT
        public:
            static IOEMBC1000 * instance();

        Q_SIGNALS:
            void pinChanged(pin_t pin, bool on);

        private:
            IOEMBC1000() {
                startTimer(10);
            }
            virtual void timerEvent(QTimerEvent *) override;
        };
#endif




        void set_alias(pin_t pin, int alias);
        void set_alias(pin_t pin, const char * alias);

        bool write(bool on, int pin_alias);
        bool write(bool on, const char * pin_alias);
        bool write(bool on, pin_t pin);

        bool read(int pin_alias);
        bool read(const char * pin_alias);
        bool read(pin_t pin);
    }

    bool init();
    void wdt_on();
    void wdt_off();
}
#endif // IOEMBC1000_H
