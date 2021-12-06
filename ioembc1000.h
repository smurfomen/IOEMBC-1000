#ifndef IOEMBC1000_H
#define IOEMBC1000_H
#include <stdint.h>

#ifdef IOEMBC1000_QT
#include <QObject>
#include <QTimerEvent>
#endif

namespace embc {

    /*! \brief  Initialization of I/O, SMBus and GPIO pins. */
    bool init();

    /*! \brief  Turn on WDT with timer settings. */
    void wdt_on(uint8_t timer);

    /*! \brief  Turn off WDT. */
    void wdt_off();

    namespace gpio {
        /*! \brief  Input pins. */
        enum ipin_t {
            GPI0 = (1 << 4),
            GPI1 = (1 << 7),
            GPI2 = (1 << 6),
            GPI3 = (1 << 0),
            GPI4 = (1 << 1),
            GPI5 = (1 << 2),
            GPI6 = (1 << 3),
            GPI7 = (1 << 5)
        };

        /*! \brief  Output pins. */
        enum opin_t {
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
        /* QObject-based class for notify messages about changes statement of input (GPI) pins */
        class GpiWatcher : public QObject {
            Q_OBJECT
        public:
            /*! \brief  Returns static instance of class IOEMBC1000. */
            static GpiWatcher * instance();

        Q_SIGNALS:
            /*!
             * \brief   Will be emitted when statement of any pin was changed.
             * \arg     pin - pin which was changed
             *          on  - current statement of pin
             */
            void pinChanged(ipin_t pin, bool on);

        private:
            GpiWatcher();
            virtual void timerEvent(QTimerEvent *) override;
        };
#endif
        /*! \brief  Writes value on to output GPO (opin_t) pin. */
        bool write(opin_t pin, bool on);

        /*! \brief  Returns current value on GPI (ipin_t) pin. */
        bool read(ipin_t pin);
    }
}
#endif // IOEMBC1000_H
