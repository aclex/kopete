#ifndef GADUADDCONTACTPAGE_H
#define GADUADDCONTACTPAGE_H

#include <qwidget.h>
#include <addcontactpage.h>

class GaduAccount;
class gaduAddUI;
class QLabel;

class GaduAddContactPage : public AddContactPage
{
    Q_OBJECT
public:
    GaduAddContactPage( GaduAccount *owner, QWidget *parent=0, const char *name=0 );
    ~GaduAddContactPage();
     virtual bool  validateData();
    virtual bool apply(KopeteAccount* , KopeteMetaContact *);
private:
    GaduAccount  *account_;
    gaduAddUI    *addUI_;
    bool          canAdd_;
    QLabel       *noaddMsg1_;
    QLabel       *noaddMsg2_;
};

#endif

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: set et ts=4 sts=4 sw=4:
 */
