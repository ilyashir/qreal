#include <QtGui>
#include <QtSql>

#include "mainwindow.h"

#include "dragwidget.h"
#include "qsqlconnectiondialog.h"
#include "realrepomodel.h"
#include "editorview.h"

MainWindow::MainWindow()
	: model(0)
{
	view = new EditorView;
	setCentralWidget(view);

	QDockWidget *diagramDock = new QDockWidget(tr("Diagram Explorer"));
	diagramExplorer = new QTreeView(this);
	diagramDock->setWidget(diagramExplorer);

	connect(diagramExplorer, SIGNAL( activated( const QModelIndex & ) ),
			view->mvIface(), SLOT( setRootIndex( const QModelIndex & ) ) );


	addDockWidget(Qt::LeftDockWidgetArea, diagramDock);

	QDockWidget *objectDock = new QDockWidget(tr("Object Explorer"));
	objectExplorer = new QTreeView(this);
	objectDock->setWidget(objectExplorer);

	addDockWidget(Qt::RightDockWidgetArea, objectDock);

    QDockWidget *dnd = new QDockWidget(tr("dnd widget"));
    DragWidget *dw = new DragWidget(this);
    dnd->setWidget(dw);
    addDockWidget(Qt::LeftDockWidgetArea, dnd);

	diagramExplorer->setDragDropMode(QAbstractItemView::DragDrop);
	objectExplorer->setDragDropMode(QAbstractItemView::DragDrop);

	createActions();
	createMenus();

	statusBar()->showMessage(tr("Yo!"));

	setWindowTitle(tr("UnREAL"));
	setMinimumSize(160, 160);
	resize(640, 480);
	
	show();
	connectRepo();
}

void MainWindow::createActions()
{
	connectAct = new QAction(tr("&Connect"), this);
	connectAct->setShortcut(tr("Ctrl+C"));
	connectAct->setStatusTip(tr("Connect to database"));
	connect(connectAct, SIGNAL(triggered()), this, SLOT(connectRepo()));

	exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcut(tr("Ctrl+Q"));
	exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
}

void MainWindow::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(connectAct);
	//    fileMenu->addAction(openAct);
	//    fileMenu->addAction(saveAct);
	//    fileMenu->addAction(printAct);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAct);
}

void MainWindow::connectRepo()
{
	QSqlConnectionDialog dialog(this);
	QSqlError err;

	if (dialog.exec() != QDialog::Accepted)
		return;

	diagramExplorer->setModel(0);
	objectExplorer->setModel(0);
	if( model )
		delete model;

	db = QSqlDatabase::addDatabase(dialog.driverName());
    
    db.setDatabaseName(dialog.databaseName());
    db.setHostName(dialog.hostName());
    db.setPort(dialog.port()); 
	
	if (!db.open(dialog.userName(), dialog.password())) {
		err = db.lastError();
		db = QSqlDatabase();
	}

	if (err.type() != QSqlError::NoError) {
		QMessageBox::warning(0, QObject::tr("Unable to open database"),
				QObject::tr("An error occured while opening the connection:\n")
				+ err.driverText() + "\n" + err.databaseText());
		return;
	}

	if ( dialog.driverName() == "QSQLITE" ) {
		this->createDatabase();
	}

	model = new RealRepoModel();
	
	diagramExplorer->setModel(model);
	diagramExplorer->setRootIndex(model->index(0,0,QModelIndex()));

	objectExplorer->setModel(model);
	//objectExplorer->setRowHidden(0,QModelIndex(),true);

	view->mvIface()->setModel(model);

	connect(objectExplorer, SIGNAL( activated( const QModelIndex & ) ),
			model, SLOT( createSomeChild( const QModelIndex & )));
}

void MainWindow::selectDiagram(const QModelIndex &index)
{
	view->mvIface()->setRootIndex(index);
}

bool MainWindow::createDatabase(){

    QFile file(":/repo/scripts.sql");
    if( !file.open(QIODevice::ReadOnly | QIODevice::Text) )
        return false;
    QTextStream in(&file);
    
    QString str;
    in >> str;
    QString all = "";
    while (!str.isEmpty() ){
        in >> str;
        all += " " + str;
    }    

    QStringList l = all.split(";"); 
    qDebug() << l.size();
    QSqlQuery q;

    for (int i=0; i<l.size(); i++){
        q.prepare(l.at(i) + ";");
        q.exec();
    }    

    file.close();
    return true;
}
