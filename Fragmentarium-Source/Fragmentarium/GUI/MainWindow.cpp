#include <QtGui>
#include <QDir>
#include <QClipboard>
#include <QDesktopServices>
#include <QImageWriter>
#include <QTextBlockUserData>
#include <QStack>
#include <QImage>
#include <QPixmap>
#include <QDialogButtonBox>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QTabWidget>
#include <QVBoxLayout>

#include "MainWindow.h"
#include "OutputDialog.h"
#include "VariableEditor.h"
#include "AnimationController.h"
#include "../../SyntopiaCore/Logging/ListWidgetLogger.h"
#include "../../SyntopiaCore/Exceptions/Exception.h"
#include "../../Fragmentarium/Parser/Preprocessor.h"
#include "../../SyntopiaCore/Math/Vector3.h"
#include "../../SyntopiaCore/Math/Random.h"
#include "../../SyntopiaCore/Math/Matrix4.h"
#include "../../ThirdPartyCode/glextensions.h"
#include "../../SyntopiaCore/Misc/Misc.h"

using namespace SyntopiaCore::Math;
using namespace SyntopiaCore::Logging;
using namespace SyntopiaCore::Exceptions;
using namespace Fragmentarium::Parser;

namespace Fragmentarium {
	namespace GUI {

		
		namespace {
			int MaxRecentFiles = 5;

			class PreferencesDialog : public QDialog
			{
			public:
				PreferencesDialog(QWidget* parent) : QDialog(parent) 
				{
					setObjectName("Dialog");
					resize(415, 335);
					setSizeGripEnabled(true);
					setModal(true);
					verticalLayout_2 = new QVBoxLayout(this);
					verticalLayout_2->setObjectName("verticalLayout_2");
					tabWidget = new QTabWidget(this);
					tabWidget->setObjectName("tabWidget");
					tab = new QWidget();
					tab->setObjectName(QString::fromUtf8("tab"));
					verticalLayout = new QVBoxLayout(tab);
					verticalLayout->setObjectName("verticalLayout");
					checkBox = new QCheckBox(tab);
					checkBox->setObjectName("checkBox");
					verticalLayout->addWidget(checkBox);

					checkBox2 = new QCheckBox(tab);
					checkBox2->setObjectName("checkBox2");
					verticalLayout->addWidget(checkBox2);

					

					QHBoxLayout* horizontalLayout2 = new QHBoxLayout();
					horizontalLayout2->setObjectName("horizontalLayout2");
					QLabel* label2 = new QLabel(tab);
					label2->setText("OpenGL refresh rate (in milliseconds)");
					label2->setObjectName("label");
					horizontalLayout2->addWidget(label2);
					refreshSpinBox = new QSpinBox(this);
					refreshSpinBox->setMaximum(100);
					refreshSpinBox->setMinimum(0);
					refreshSpinBox->setValue(20);
					horizontalLayout2->addWidget(refreshSpinBox);
					verticalLayout->addLayout(horizontalLayout2);


					horizontalLayout = new QHBoxLayout();
					horizontalLayout->setObjectName("horizontalLayout");
					label = new QLabel(tab);
					label->setObjectName("label");
					horizontalLayout->addWidget(label);
					lineEdit = new QLineEdit(tab);
					lineEdit->setObjectName("lineEdit");
					horizontalLayout->addWidget(lineEdit);
					verticalLayout->addLayout(horizontalLayout);

					verticalSpacer = new QSpacerItem(20, 167, QSizePolicy::Minimum, QSizePolicy::Expanding);
					verticalLayout->addItem(verticalSpacer);

					tabWidget->addTab(tab, QString());
					verticalLayout_2->addWidget(tabWidget);

					buttonBox = new QDialogButtonBox(this);
					buttonBox->setObjectName("buttonBox");
					buttonBox->setOrientation(Qt::Horizontal);
					buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

					verticalLayout_2->addWidget(buttonBox);
					QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
					QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
					tabWidget->setCurrentIndex(0);
					QMetaObject::connectSlotsByName(this);
					setWindowTitle("Preferences");
					checkBox->setText("Move main() to end");
					checkBox->setStatusTip(tr("For compatibility with some GPU's."));

					checkBox2->setText("Replace all 'float' types with 'double' types. (Not working!)");
					checkBox2->setStatusTip(tr("Also replaces matrix and vector types."));

					

					label->setText("Include paths:");
					tabWidget->setTabText(tabWidget->indexOf(tab),  "Main");

					QSettings settings;
					checkBox->setChecked(settings.value("moveMain", true).toBool());
					checkBox2->setChecked(settings.value("doublify", false).toBool());
					refreshSpinBox->setValue(settings.value("refreshRate", 20).toInt());
					lineEdit->setText(settings.value("includePaths", "Examples/Include;").toString());
				} 

				virtual void accept() {
					saveSettings();
					QDialog::accept();
				}

				void saveSettings() {
					QSettings settings;
					settings.setValue("moveMain", checkBox->isChecked());
					settings.setValue("includePaths", lineEdit->text());
					settings.setValue("doublify",  checkBox2->isChecked());
					settings.setValue("refreshRate",  refreshSpinBox->value());
				}

				QVBoxLayout *verticalLayout_2;
				QTabWidget *tabWidget;
				QWidget *tab;
				QVBoxLayout *verticalLayout;
				QCheckBox *checkBox;
				QCheckBox *checkBox2;
				QHBoxLayout *horizontalLayout;
				QLabel *label;
				QLineEdit *lineEdit;
				QSpacerItem *verticalSpacer;
				QWidget *tab_2;
				QDialogButtonBox *buttonBox;
				QSpinBox* refreshSpinBox ;
			};

		}

		namespace {

			void createCommandHelpMenu(QMenu* menu, QWidget* textEdit, MainWindow* mainWindow) {
				QMenu *preprocessorMenu = new QMenu("Host Preprocessor Commands", 0);
				preprocessorMenu->addAction("#include \"some.frag\"", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#camera 3D", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#camera 2D", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#info sometext", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#group parameter_group_name", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#preset preset_name", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#endpreset", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#define DontClearOnChange", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#define IterationsBetweenRedraws 10", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#define SubframeMax 20", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#TexParameter textureName GL_TEXTURE_MAG_FILTER GL_NEAREST", textEdit , SLOT(insertText()));
				preprocessorMenu->addAction("#TexParameter textureName GL_TEXTURE_WRAP_S GL_CLAMP", textEdit , SLOT(insertText()));
				



				QMenu *uniformMenu = new QMenu("Special Uniforms", 0);
				uniformMenu->addAction("uniform float time;", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform vec2 pixelSize;", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform int i; slider[0,1,2]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform float f; slider[0,1,2]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform vec2 v; slider[(0,0),(1,1),(1,1)]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform vec3 v; slider[(0,0,0),(1,1,1),(1,1,1)]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform vec4 v; slider[(0,0,0,0),(1,1,1,1),(1,1,1,1)]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform bool b; checkbox[true]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform sampler2D tex; file[tex.jpg]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform vec3 color; color[0.0,0.0,0.0]", textEdit , SLOT(insertText()));
				uniformMenu->addAction("uniform vec4 color; color[0.0,1.0,0.0,0.0,0.0,0.0]", textEdit , SLOT(insertText()));

				QMenu *presetMenu = new QMenu("Presets", 0);
				presetMenu->addAction("Insert Preset from Current Settings",mainWindow, SLOT(insertPreset()));
				
				QMenu *includeMenu = new QMenu("Include (from Preferences Paths)", 0);
				
				QStringList filter; filter << "*.frag";
				QSettings settings;
				QStringList includePaths = settings.value("includePaths", "Examples/Include;").toString().split(";", QString::SkipEmptyParts);
				mainWindow->getFileManager()->setIncludePaths(includePaths);
				QStringList files = mainWindow->getFileManager()->getFiles(filter);
				foreach (QString s, files) {
					includeMenu->addAction(QString("#include \"%1\"").arg(s),mainWindow, SLOT(insertText()));
				}

				QAction* before = 0;
				if (menu->actions().count() > 0) before = menu->actions()[0];
				menu->insertMenu(before, preprocessorMenu);
				menu->insertMenu(before, uniformMenu);
				menu->insertMenu(before, presetMenu);
				menu->insertMenu(before, includeMenu);
			
				menu->insertSeparator(before);
			}
		}
		void TextEdit::contextMenuEvent(QContextMenuEvent *event)
		{	
			QMenu *menu = createStandardContextMenu();
			createCommandHelpMenu(menu, this, mainWindow);
			menu->exec(event->globalPos());
			delete menu;
		}

		void TextEdit::insertText() {
			QString text = ((QAction*)sender())->text();
			insertPlainText(text.section("//",0,0)); // strip comments
		}

		
		void TextEdit::insertFromMimeData ( const QMimeData * source )
		{
			QTextEdit::insertPlainText ( source->text() );
		}


		class FragmentHighlighter : public QSyntaxHighlighter {
		public:
			FragmentHighlighter(QTextEdit* e) : QSyntaxHighlighter(e) {
				keywordFormat.setFontWeight(QFont::Bold);
				keywordFormat.setForeground(Qt::darkMagenta);
				bracketFormat.setFontWeight(QFont::Bold);
				bracketFormat.setForeground(Qt::blue);
				primitiveFormat.setFontWeight(QFont::Bold);
				primitiveFormat.setForeground(Qt::darkYellow);
				commentFormat.setForeground(Qt::darkGreen);
				warningFormat.setBackground(QBrush(Qt::yellow));
				preprocessorFormat.setForeground(QBrush(Qt::blue));
				preprocessorFormat.setFontWeight(QFont::Bold);
				preprocessor2Format.setForeground(QBrush(Qt::darkRed));
				preprocessor2Format.setFontWeight(QFont::Bold);

				expression = QRegExp("(uniform|attribute|bool|break|bvec2|bvec3|bvec4|const|continue|discard|do|else|false|float|for|if|in|inout|int|ivec2|ivec3|ivec4|main|mat2|mat3|mat4|out|return|true|uniorm|varying|vec2|vec3|vec4|void|while|sampler1D|sampler2D|sampler3Dsampler2DShadow|struct)");
				primitives = QRegExp("(abs|acos|all|any|asin|atan|ceil|sin|clamp|cos|cross|dFdx|dFdy|degrees|distance|dot|equal|exp|exp2|faceforward|floor|fract|ftransform|fwidth|greaterThan|greaterThanqual|inversesqrt|length|lessThan|lessThanEqual|log|log2|matrixCompMult|max|min|mix|mod|noise1|noise2|noise3|noise4|normalize|not|notEqual|pow|radians|reflect|reract|shadow1D|shadow1DLod|shadow1DProj|shadow1DProjLod|shadow2D|shadow2DLod|shadow2DProj|shadow2DProjLod|sign|sin|smoothstep|sqrt|step|tan|texture1D|texture1DLod|texture1DProj|texture1DProjLod|texture2D|texture2DLod|texture2DProj|texture2DProjLod|texture3D|texture3DLod|texture3DProj|texture3DProjLod|textureCube|textureCubeLod)");
				randomNumber = QRegExp("(random\\[[-+]?[0-9]*\\.?[0-9]+,[-+]?[0-9]*\\.?[0-9]+\\])"); // random[-2.3,3.4]

				float4Slider = QRegExp("^\\s*uniform\\s+vec4\\s+(\\S+)\\s*;\\s*slider\\[\\((\\S+),(\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+),(\\S+)\\)\\].*$");
				float3Slider = QRegExp("^\\s*uniform\\s+vec3\\s+(\\S+)\\s*;\\s*slider\\[\\((\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+)\\)\\].*$");
				float2Slider = QRegExp("^\\s*uniform\\s+vec2\\s+(\\S+)\\s*;\\s*slider\\[\\((\\S+),(\\S+)\\),\\((\\S+),(\\S+)\\),\\((\\S+),(\\S+)\\)\\].*$"); 
				colorChooser = QRegExp("^\\s*uniform\\s+vec3\\s+(\\S+)\\s*;\\s*color\\[(\\S+),(\\S+),(\\S+)\\].*$"); 
				floatColorChooser = QRegExp("^\\s*uniform\\s+vec4\\s+(\\S+)\\s*;\\s*color\\[(\\S+),(\\S+),(\\S+),(\\S+),(\\S+),(\\S+)\\].*$"); 
				floatSlider = QRegExp("^\\s*uniform\\s+float\\s+(\\S+)\\s*;\\s*slider\\[(\\S+),(\\S+),(\\S+)\\].*$"); 
				intSlider = QRegExp("^\\s*uniform\\s+int\\s+(\\S+)\\s*;\\s*slider\\[(\\S+),(\\S+),(\\S+)\\].*$"); 
				boolChooser = QRegExp("^\\s*uniform\\s+bool\\s+(\\S+)\\s*;\\s*checkbox\\[(\\S+)\\].*$"); 
				replace = QRegExp("^#replace\\s+\"([^\"]+)\"\\s+\"([^\"]+)\"\\s*$"); // Look for #reaplace "var1" "var2"
				sampler2D = QRegExp("^\\s*uniform\\s+sampler2D\\s+(\\S+)\\s*;\\s*file\\[(.*)\\].*$"); 

				expression.setCaseSensitivity(Qt::CaseInsensitive);
				primitives.setCaseSensitivity(Qt::CaseInsensitive);
				randomNumber.setCaseSensitivity(Qt::CaseInsensitive);
			};

			void highlightBlock(const QString &text)
			{

				if (currentBlockState() == 2) {
					setFormat(0, text.length(), warningFormat);
					setCurrentBlockState(-1);
					return;
				}

				if (previousBlockState() != 1 && currentBlockState() == 1) {
					// This line was previously a multi-line start 
					if (!text.contains("*/")) setCurrentBlockState(0);
				}

				if (previousBlockState() == 1) {
					// Part of multi-line comment. Skip the rest...
					if (!text.contains("*/")) {
						setFormat(0, text.length(), commentFormat);
						setCurrentBlockState(1);
						return;
					}
				}

				// Line parsing
				QString current;
				int startMatch = 0;				
				if (float2Slider.exactMatch(text) || float3Slider.exactMatch(text) || float4Slider.exactMatch(text) || colorChooser.exactMatch(text) || floatSlider.exactMatch(text) ||
					intSlider.exactMatch(text) || boolChooser.exactMatch(text) || replace.exactMatch(text) ||
					sampler2D.exactMatch(text) || floatColorChooser.exactMatch(text)) {
						setFormat(0, text.length()-1, preprocessor2Format);
				}

				for (int i = 0; i < text.length(); i++) {
					if ((i > 0) && text.at(i) == '*' && text.at(i-1) == '/') {
						// Multi-line comment begins
						setFormat(i-1, text.length()-i+1, commentFormat);
						setCurrentBlockState(1);
						return;
					}

					if ((i > 0) && text.at(i) == '/' && text.at(i-1) == '*') {
						// Multi-line comment ends
						setFormat(0, i, commentFormat);
						if (currentBlockState() != 0) {
							setCurrentBlockState(0);
						}
						continue;
					}

					if (text.at(0) == '#') {
						// Preprocessor format
						setFormat(0, text.length(), preprocessorFormat);
						continue;
					}

					if ((i > 0) && (i < text.length()-2) && text.at(i) == '/' && text.at(i-1) == '/') {
						// Single-line comments
						setFormat(i-1, text.length()-i+1, commentFormat);
						break;
					}

					bool delimiter = !text.at(i).isLetterOrNumber();//(text.at(i) == '(' || text.at(i) == ')' || text.at(i) == '{' || text.at(i) == '\t' || text.at(i) == '}' || text.at(i) == ' '  || (text.at(i) == '\r') || (text.at(i) == '\n'));
					bool lastChar = (i==text.length()-1);
					if (delimiter || lastChar) {
						if (lastChar && !delimiter) current += text.at(i);
						int adder = (i==text.length()-1 ? 1 : 0);
						if (expression.exactMatch(current)) setFormat(startMatch, i-startMatch+adder, keywordFormat);
						if (primitives.exactMatch(current)) setFormat(startMatch, i-startMatch+adder, primitiveFormat);
						if (randomNumber.exactMatch(current)) setFormat(startMatch, i-startMatch+adder, preprocessorFormat);
						if (text.at(i) == '{' || text.at(i) == '}') setFormat(i, 1, bracketFormat);
						startMatch = i;
						current = "";
					} else {
						current += text.at(i);
					}
				}
			}; 

		private:
			QTextCharFormat keywordFormat;
			QTextCharFormat bracketFormat;
			QTextCharFormat primitiveFormat;
			QTextCharFormat commentFormat;
			QTextCharFormat warningFormat;
			QTextCharFormat preprocessorFormat;
			QTextCharFormat preprocessor2Format;

			QRegExp float4Slider;
			QRegExp float3Slider;
			QRegExp float2Slider;
			QRegExp colorChooser;
			QRegExp floatSlider;
			QRegExp intSlider;
			QRegExp boolChooser;
			QRegExp replace;
			QRegExp sampler2D;
			QRegExp floatColorChooser;

			QRegExp expression;
			QRegExp primitives;
			QRegExp randomNumber;
		};


		MainWindow::MainWindow()
		{
			init();
			loadFile(QDir(getExamplesDir()).absoluteFilePath("Historical 3D Fractals/Mandelbulb.frag"));
			tabChanged(0); // to update title.
		}

		MainWindow::MainWindow(const QString &fileName)
		{
			QDir::setCurrent(QCoreApplication::applicationDirPath ()); // Otherwise we cannot find examples + templates
			init();
			loadFile(fileName);
			tabChanged(0); // to update title.
		}

		void MainWindow::closeEvent(QCloseEvent *ev)
		{

			bool modification = false;
			for (int i = 0; i < tabInfo.size(); i++) {
				if (tabInfo[i].unsaved) modification = true;
			}

			if (modification) {
				int i = QMessageBox::warning(this, "Unsaved changed", "There are tabs with unsaved changes.\r\nContinue and loose changes?", QMessageBox::Ok, QMessageBox::Cancel);
				if (i == QMessageBox::Ok) {
					// OK
					ev->accept();
					return;
				} else {
					// Cancel
					ev->ignore();
					return;
				}
			}
			ev->accept();

			writeSettings();
		}

		void MainWindow::newFile()
		{
			insertTabPage("");
		}

		void MainWindow::insertPreset() {
			getTextEdit()->insertPlainText("\n\n#preset Noname\n" + variableEditor->getSettings()+ "\n#endpreset\n"); 
		}

		void MainWindow::open()
		{
			QString filter = "Fragment Source (*.frag);;All Files (*.*)";
			QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(), filter);
			if (!fileName.isEmpty()) {
				loadFile(fileName);
			}
		}

		void MainWindow::keyReleaseEvent(QKeyEvent* ev) {
			if (ev->key() == Qt::Key_Escape) {
				toggleFullScreen();
			} else if (ev->key() == Qt::Key_F5 && fullScreenEnabled) {
				render();
			} else if (ev->key() == Qt::Key_F6) {
				callRedraw();
			}  else {
				ev->ignore();
			}
		};


		bool MainWindow::save()
		{
			int index = tabBar->currentIndex();
			if (index == -1) { WARNING("No open tab"); return false; } 
			TabInfo t = tabInfo[index];

			if (t.hasBeenSavedOnce) {
				return saveFile(t.filename);
			} else {
				return saveAs();
			}
		}

		bool MainWindow::saveAs()
		{
			int index = tabBar->currentIndex();
			if (index == -1) { WARNING("No open tab"); return false; } 

			TabInfo t = tabInfo[index];

			QString filter = "Fragment Source (*.frag);;All Files (*.*)";

			QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), t.filename, filter);
			if (fileName.isEmpty())
				return false;

			return saveFile(fileName);
		}

		void MainWindow::about()
		{
			QFile file(getMiscDir() + QDir::separator() + "about.html");
			if (!file.open(QFile::ReadOnly | QFile::Text)) {
				WARNING("Could not open 'about.html'.");
				return;
			}

			QTextStream in(&file);
			QString text = in.readAll();
			text.replace("$VERSION$", version.toLongString());

			QMessageBox mb(this);
			mb.setText(text);
			mb.setWindowTitle("About Fragmentarium");
			mb.setIconPixmap(getMiscDir() + QDir::separator() + "icon.jpg");
			mb.setMinimumWidth(800);
			mb.exec();
		}

		void MainWindow::showControlHelp()
		{
			QFile file(getMiscDir() + QDir::separator() + "control.html");
			if (!file.open(QFile::ReadOnly | QFile::Text)) {
				WARNING("Could not open 'control.html'.");
				return;
			}

			QTextStream in(&file);
			QString text = in.readAll();

			QMessageBox mb(this);
			mb.setText(text);
			mb.setWindowTitle("Mouse and Keyboard Control");
			mb.setIconPixmap(getMiscDir() + QDir::separator() + "icon.jpg");
			mb.setMinimumWidth(800);
			mb.exec();
		}

		void MainWindow::documentWasModified()
		{
			tabInfo[tabBar->currentIndex()].unsaved = true;
			tabChanged(tabBar->currentIndex());
		}

		void MainWindow::init()
		{
			setAcceptDrops(true);

			rebuildRequired = false;

			hasBeenResized = true;

			oldDirtyPosition = -1;
			setFocusPolicy(Qt::StrongFocus);

			version = SyntopiaCore::Misc::Version(0, 9, 1, -1, " (\"Chiaroscuro\")");
			setAttribute(Qt::WA_DeleteOnClose);

			QSplitter*	splitter = new QSplitter(this);
			splitter->setObjectName(QString::fromUtf8("splitter"));
			splitter->setOrientation(Qt::Horizontal);

			stackedTextEdits = new QStackedWidget(splitter);

			QGLFormat format;
			format.setDoubleBuffer(false);
			engine = new DisplayWidget(format, this,splitter);
			engine->makeCurrent();
			engine->updateRefreshRate();
			if (!getGLExtensionFunctions().resolve(engine->context())) {
				QMessageBox::critical(0, "OpenGL features missing",
					"Failed to resolve OpenGL functions required to run this application.\n"
					"The program will now exit.");
				exit(0);
			}
			tabBar = new QTabBar(this);

#if QT_VERSION >= 0x040500
			tabBar->setTabsClosable(true);
			connect(tabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
#endif 

			fpsLabel = new QLabel(this);
			statusBar()->addPermanentWidget(fpsLabel);

			QFrame* f = new QFrame(this);
			frameMainWindow = new QVBoxLayout();
			frameMainWindow->setSpacing(0);
			frameMainWindow->setMargin(4);
			f->setLayout(frameMainWindow);
			f->layout()->addWidget(tabBar);
			f->layout()->addWidget(splitter);
			setCentralWidget(f);

			QList<int> l; l.append(100); l.append(400);
			splitter->setSizes(l);

			createActions();

			QDir d(getExamplesDir());

			// Log widget (in dockable window)
			dockLog = new QDockWidget(this);
			dockLog->setWindowTitle("Log");
			dockLog->setObjectName(QString::fromUtf8("dockWidget"));
			dockLog->setAllowedAreas(Qt::BottomDockWidgetArea);
			QWidget* dockLogContents = new QWidget(dockLog);
			dockLogContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
			QVBoxLayout* vboxLayout1 = new QVBoxLayout(dockLogContents);
			vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
			vboxLayout1->setContentsMargins(0, 0, 0, 0);

			logger = new ListWidgetLogger(dockLog);
			vboxLayout1->addWidget(logger->getListWidget());
			dockLog->setWidget(dockLogContents);
			addDockWidget(static_cast<Qt::DockWidgetArea>(8), dockLog);

			// Variable editor (in dockable window)
			editorDockWidget = new QDockWidget(this);
			editorDockWidget->setMinimumWidth(250);
			editorDockWidget->setWindowTitle("Parameters (uniforms)");
			editorDockWidget->setObjectName(QString::fromUtf8("editorDockWidget"));
			editorDockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
			QWidget* editorLogContents = new QWidget(dockLog);
			editorLogContents->setObjectName(QString::fromUtf8("editorLogContents"));
			QVBoxLayout* vboxLayout2 = new QVBoxLayout(editorLogContents);
			vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
			vboxLayout2->setContentsMargins(0, 0, 0, 0);

			variableEditor = new VariableEditor(editorDockWidget, this);
			variableEditor->setMinimumWidth(250);
			vboxLayout2->addWidget(variableEditor);
			editorDockWidget->setWidget(editorLogContents);
			addDockWidget(Qt::RightDockWidgetArea, editorDockWidget);
			connect(variableEditor, SIGNAL(changed(bool)), this, SLOT(variablesChanged(bool)));

			editorDockWidget->setHidden(true);
			setMouseTracking(true);

			INFO(QString("Welcome to Fragmentarium version %1. A Syntopia Project.").arg(version.toLongString()));
			INFO("");
			WARNING("This is an experimental SVN checkout build. For stability use the package releases.");

			fullScreenEnabled = false;
			createOpenGLContextMenu();

			connect(this->tabBar, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

			readSettings();

			createToolBars();
			createStatusBar();
			createMenus();

			animationController = new AnimationController(this);
			animationController->setAllowedAreas(Qt::BottomDockWidgetArea);
			addDockWidget(Qt::BottomDockWidgetArea, animationController, Qt::Vertical);
			animationController->setFloating(true);
			connect(((AnimationController*)animationController)->getAnimationSettings(), SIGNAL(timeUpdated()), this, SLOT(callRedraw()));
			connect(animationController, SIGNAL(wasHidden()), this, SLOT(animationControllerHidden()));

			renderModeChanged();

		}

		void MainWindow::animationControllerHidden() {
			INFO("Animation Controller closed. Switching to automatic render mode.");
			renderModeChanged();

		}

		void MainWindow::setUserUniforms(QGLShaderProgram* shaderProgram) {
			if (!variableEditor || !shaderProgram) return;
			variableEditor->setUserUniforms(shaderProgram);
		}

		void MainWindow::variablesChanged(bool lockedChanged) {
			if (lockedChanged) {
				highlightBuildButton(true);
			}
			engine->uniformsHasChanged();
		}


		void MainWindow::createOpenGLContextMenu() {
			openGLContextMenu = new QMenu();			
			openGLContextMenu->addAction(fullScreenAction);
			openGLContextMenu->addAction(screenshotAction);
			engine->setContextMenu(openGLContextMenu);
		}


		void MainWindow::toggleFullScreen() {
			if (fullScreenEnabled) {
				frameMainWindow->setMargin(4);
				showNormal();
				fullScreenEnabled = false;
				fullScreenAction->setChecked(false);
				stackedTextEdits->show();
				dockLog->show();
				menuBar()->show();
				statusBar()->show();
				fileToolBar->show();
				editToolBar->show();
				renderToolBar->show();
				tabBar->show();
				renderModeToolBar->show();
			} else {
				frameMainWindow->setMargin(0);
				fullScreenAction->setChecked(true);
				fullScreenEnabled = true;

				tabBar->hide();
				stackedTextEdits->hide();
				dockLog->hide();
				menuBar()->hide();
				statusBar()->hide();
				fileToolBar->hide();
				editToolBar->hide();
				renderToolBar->hide();
				renderModeToolBar->hide();
				showFullScreen();
			}
		}


		void MainWindow::createActions()
		{
			fullScreenAction = new QAction(tr("F&ullscreen"), this);
			fullScreenAction->setShortcut(tr("Ctrl+F"));
			fullScreenAction->setCheckable(true);
			connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));

			screenshotAction = new QAction(tr("&Save as Bitmap..."), this);
			connect(screenshotAction, SIGNAL(triggered()), this, SLOT(makeScreenshot()));

			newAction = new QAction(QIcon(":/Icons/new.png"), tr("&New"), this);
			newAction->setShortcut(tr("Ctrl+N"));
			newAction->setStatusTip(tr("Create a new file"));
			connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

			openAction = new QAction(QIcon(":/Icons/open.png"), tr("&Open..."), this);
			openAction->setShortcut(tr("Ctrl+O"));
			openAction->setStatusTip(tr("Open an existing file"));
			connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

			saveAction = new QAction(QIcon(":/Icons/save.png"), tr("&Save"), this);
			saveAction->setShortcut(tr("Ctrl+S"));
			saveAction->setStatusTip(tr("Save the script to disk"));
			connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

			saveAsAction = new QAction(QIcon(":/Icons/filesaveas.png"), tr("Save &As..."), this);
			saveAsAction->setStatusTip(tr("Save the script under a new name"));
			connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

			closeAction = new QAction(QIcon(":/Icons/fileclose.png"), tr("&Close Tab"), this);
			closeAction->setShortcut(tr("Ctrl+W"));
			closeAction->setStatusTip(tr("Close this tab"));
			connect(closeAction, SIGNAL(triggered()), this, SLOT(closeTab()));

			exitAction = new QAction(QIcon(":/Icons/exit.png"), tr("E&xit Application"), this);
			exitAction->setShortcut(tr("Ctrl+Q"));
			exitAction->setStatusTip(tr("Exit the application"));
			connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

			cutAction = new QAction(QIcon(":/Icons/cut.png"), tr("Cu&t"), this);
			cutAction->setShortcut(tr("Ctrl+X"));
			cutAction->setStatusTip(tr("Cut the current selection's contents to the "
				"clipboard"));
			connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));

			copyAction = new QAction(QIcon(":/Icons/copy.png"), tr("&Copy"), this);
			copyAction->setShortcut(tr("Ctrl+C"));
			copyAction->setStatusTip(tr("Copy the current selection's contents to the "
				"clipboard"));
			connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

			pasteAction = new QAction(QIcon(":/Icons/paste.png"), tr("&Paste"), this);
			pasteAction->setShortcut(tr("Ctrl+V"));
			pasteAction->setStatusTip(tr("Paste the clipboard's contents into the current "
				"selection"));
			connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

			renderAction = new QAction(QIcon(":/Icons/render.png"), tr("&Build System"), this);
			renderAction->setShortcut(tr("F5"));
			renderAction->setStatusTip(tr("Render the current ruleset"));
			connect(renderAction, SIGNAL(triggered()), this, SLOT(render()));

			aboutAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&About"), this);
			aboutAction->setStatusTip(tr("Shows the About box"));
			connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

			controlAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&Mouse and Keyboard Help"), this);
			controlAction->setStatusTip(tr("Shows information about how to control Fragmentarium"));
			connect(controlAction, SIGNAL(triggered()), this, SLOT(showControlHelp()));

			sfHomeAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&Project Homepage (web link)"), this);
			sfHomeAction->setStatusTip(tr("Open the project page in a browser."));
			connect(sfHomeAction, SIGNAL(triggered()), this, SLOT(launchSfHome()));

			referenceAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&Fragmentarium Reference (web link)"), this);
			referenceAction->setStatusTip(tr("Open a Fragmentarium reference web page in a browser."));
			connect(referenceAction, SIGNAL(triggered()), this, SLOT(launchReferenceHome()));

			galleryAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&Flickr Fragmentarium Group (web link)"), this);
			galleryAction->setStatusTip(tr("Opens the main Flickr group for Fragmentarium creations."));
			connect(galleryAction, SIGNAL(triggered()), this, SLOT(launchGallery()));

			glslHomeAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&GLSL Specifications (web link)"), this);
			glslHomeAction->setStatusTip(tr("The official specifications for all GLSL versions."));
			connect(glslHomeAction, SIGNAL(triggered()), this, SLOT(launchGLSLSpecs()));

			introAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("Introduction to Distance Estimated Fractals (web link)"), this);
			connect(introAction, SIGNAL(triggered()), this, SLOT(launchIntro()));

			faqAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("Fragmentarium FAQ (web link)"), this);
			connect(faqAction, SIGNAL(triggered()), this, SLOT(launchFAQ()));


			for (int i = 0; i < MaxRecentFiles; ++i) {
				QAction* a = new QAction(this);
				a->setVisible(false);
				connect(a, SIGNAL(triggered()),	this, SLOT(openFile()));
				recentFileActions.append(a);				
			}

			qApp->setWindowIcon(QIcon(":/Icons/fragmentarium.png"));
		}

		void MainWindow::createMenus()
		{
			// -- File Menu --
			fileMenu = menuBar()->addMenu(tr("&File"));
			fileMenu->addAction(newAction);
			fileMenu->addAction(openAction);
			fileMenu->addAction(saveAction);
			fileMenu->addAction(saveAsAction);
			recentFileSeparator = fileMenu->addSeparator();
			for (int i = 0; i < MaxRecentFiles; ++i) fileMenu->addAction(recentFileActions[i]);
			fileMenu->addSeparator();
			fileMenu->addAction(closeAction);
			fileMenu->addAction(exitAction);

			// -- Edit Menu --
			editMenu = menuBar()->addMenu(tr("&Edit"));
			editMenu->addAction(cutAction);
			editMenu->addAction(copyAction);
			editMenu->addAction(pasteAction);
			editMenu->addSeparator();
			editMenu->addAction("Indent Script", this, SLOT(indent()));
			QMenu* m = editMenu->addMenu("Insert Command");
			createCommandHelpMenu(m, this, this);
			editMenu->addSeparator();
			editMenu->addAction("Preferences...", this, SLOT(preferences()));


			// -- Render Menu --
			renderMenu = menuBar()->addMenu(tr("&Render"));
			renderMenu->addAction(renderAction);
			renderMenu->addAction("High Resolution Render", this, SLOT(tileBasedRender()));
			renderMenu->addSeparator();
			renderMenu->addAction("Output Preprocessed Script (for Debug)", this, SLOT(showDebug()));
			renderMenu->addSeparator();
			renderMenu->addAction(fullScreenAction);


			// -- Render Menu --
			QMenu* parametersMenu = menuBar()->addMenu(tr("&Parameters"));
			parametersMenu->addAction("Reset All", variableEditor, SLOT(resetUniforms()), QKeySequence("F1"));
			parametersMenu->addSeparator();
			parametersMenu->addAction("Copy to Clipboard", variableEditor, SLOT(copy()), QKeySequence("F2"));
			parametersMenu->addAction("Paste from Clipboard", variableEditor, SLOT(paste()), QKeySequence("F3"));
			parametersMenu->addAction("Paste from Selected Text", this, SLOT(pasteSelected()), QKeySequence("F4"));
			parametersMenu->addSeparator();
			parametersMenu->addAction("Save to File", this, SLOT(saveParameters()));
			parametersMenu->addAction("Load from File", this, SLOT(loadParameters()));
			parametersMenu->addSeparator();

			// -- Examples Menu --
			QStringList filters;
			QMenu* examplesMenu = menuBar()->addMenu(tr("&Examples"));
			// Scan examples dir...
			QDir d(getExamplesDir());
			filters.clear();
			filters << "*.frag";
			d.setNameFilters(filters);
			if (!d.exists()) {
				QAction* a = new QAction("Unable to locate: "+d.absolutePath(), this);
				a->setEnabled(false);
				examplesMenu->addAction(a);
			} else {
				// we will recurse the dirs...
				QStack<QString> pathStack;
				pathStack.append(QDir(getExamplesDir()).absolutePath());

				QMap< QString , QMenu* > menuMap;
				while (!pathStack.isEmpty()) {

					QMenu* currentMenu = examplesMenu;
					QString path = pathStack.pop();
					if (menuMap.contains(path)) currentMenu = menuMap[path];
					QDir dir(path);

					QStringList sl = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
					for (int i = 0; i < sl.size(); i++) {
						QMenu* menu = new QMenu(sl[i]);
						QString absPath = QDir(path + QDir::separator() +  sl[i]).absolutePath();
						menuMap[absPath] = menu;
						currentMenu->addMenu(menu);
						menu->setIcon(QIcon(":/Icons/folder.png"));
						pathStack.push(absPath);
					}

					dir.setNameFilters(filters);

					sl = dir.entryList();
					for (int i = 0; i < sl.size(); i++) {
						QAction* a = new QAction(sl[i], this);
						a->setIcon(QIcon(":/Icons/mail_new.png"));


						QString absPath = QDir(path ).absoluteFilePath(sl[i]);

						a->setData(absPath);
						connect(a, SIGNAL(triggered()), this, SLOT(openFile()));
						currentMenu->addAction(a);
					}
				}
			}

			QMenu* mc = createPopupMenu();
			mc->setTitle("Windows");
			menuBar()->addMenu(mc);

			helpMenu = menuBar()->addMenu(tr("&Help"));
			helpMenu->addAction(aboutAction);
			helpMenu->addAction(controlAction);

			helpMenu->addSeparator();
			helpMenu->addAction(sfHomeAction);
			helpMenu->addAction(galleryAction);
			helpMenu->addAction(glslHomeAction);
			helpMenu->addAction(faqAction);
			helpMenu->addAction(introAction);
		}

		void MainWindow::tileBasedRender() {
			OutputDialog od(this, engine->width(), engine->height());
			if (od.exec() == QDialog:: Accepted) {
				if (od.doSaveFragment()) {
					QString fileName = od.getFragmentFileName();
					logger->getListWidget()->clear();
					if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; }
					QString inputText = getTextEdit()->toPlainText();
					QSettings settings;
					QStringList includePaths = settings.value("includePaths", "Examples/Include;").toString().split(";", QString::SkipEmptyParts);
					fileManager.setIncludePaths(includePaths);
					Preprocessor p(&fileManager);
					try {
						QString file = tabInfo[tabBar->currentIndex()].filename;
						FragmentSource fs = p.createAutosaveFragment(inputText,file);
						QString prepend =  "// Output generated from file: " + file + "\n";
						prepend += "// Created: " + QDateTime::currentDateTime().toString() + "\n";
						QString append = "\n\n#preset default\n" + variableEditor->getSettings() + "\n#endpreset\n\n";
						QString final = prepend + fs.getText() + append;

						
						QString f = od.getFileName();
						QDir oDir(QFileInfo(f).absolutePath());
						QString subdirName = f + " Files";
						if (!oDir.mkdir(subdirName)) {

							QMessageBox::warning(this, tr("Fragmentarium"),
								tr("Could not create directory %1:\n.")
								.arg(oDir.filePath(subdirName)));
							return;
						}
						subdirName = oDir.filePath(subdirName); // full name
						

						QFile fileStream(subdirName + "/" + fileName);
						if (!fileStream.open(QFile::WriteOnly | QFile::Text)) {
							QMessageBox::warning(this, tr("Fragmentarium"),
								tr("Cannot write file %1:\n%2.")
								.arg(fileName)
								.arg(fileStream.errorString()));
							return;
						}

						QTextStream out(&fileStream);
						out << final;
						INFO("Saved fragment + settings as: " + fileName);
	
						// Copy files.
						QStringList ll = p.getDependencies();
						foreach (QString l, ll) {
							QString from = l;
							QString to =  QDir(subdirName).absoluteFilePath( QFileInfo(l).fileName() );
							if (!QFile::copy(from,to)) {
								QMessageBox::warning(this, tr("Fragmentarium"),
									tr("Could not copy dependency:\n'%1' to \n'%2'.")
								.arg(from)
								.arg(to));
							return;
							}

						}
					} catch (Exception& e) {
						WARNING(e.getMessage());
					}
				}
				engine->setupTileRender(od.getTiles(),od.getFrames(), od.getFileName());
			};			
		}

		TileRenderDialog::TileRenderDialog(QWidget* parent, int w, int h) : QDialog(parent), w(w), h(h) {
			setWindowTitle("Select number of tiles");
			QVBoxLayout* layout = new QVBoxLayout(this);
			tileLabel = new QLabel("Resolution: XxX", this);
			layout->addWidget(tileLabel);
			tileSlider = new QSlider(Qt::Horizontal, this);
			layout->addWidget(tileSlider);
			tileSlider->setMinimum(1);
			tileSlider->setValue(3);
			tileSlider->setMaximum(20);
			connect(tileSlider, SIGNAL(valueChanged(int)), this, SLOT(tilesChanged(int)));
			tilesChanged(0);

			QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			layout->addWidget(buttonBox);
			connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
			connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
		}

		void TileRenderDialog::tilesChanged(int) {
			int t = tileSlider->value();
			tileLabel->setText(QString("Resolution: %1x%2 (%3 Megapixels)").arg(t*w).arg(t*h).arg((double)((t*w*t*h)/(1024.0*1024.0)),0,'f',1));
		}

		void MainWindow::pasteSelected() {
			QString settings = getTextEdit()->textCursor().selectedText();	
			// Note: If the selection obtained from an editor spans a line break,
			// the text will contain a Unicode U+2029 paragraph separator character instead of a newline \n character. Use QString::replace() to replace these characters with newlines
			settings = settings.replace(QChar::ParagraphSeparator,"\n");
			variableEditor->setSettings(settings);
			statusBar()->showMessage(tr("Pasted selected settings"), 2000);
		}

		void MainWindow::saveParameters() {
			QString filter = "Fragment Parameters (*.fragparams);;All Files (*.*)";
			QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), "", filter);
			if (fileName.isEmpty())
				return;

			QFile file(fileName);
			if (!file.open(QFile::WriteOnly | QFile::Text)) {
				QMessageBox::warning(this, tr("Fragmentarium"),
					tr("Cannot write file %1:\n%2.")
					.arg(fileName)
					.arg(file.errorString()));
				return;
			}

			QTextStream out(&file);
			out << variableEditor->getSettings();
			statusBar()->showMessage(tr("Settings saved to file"), 2000);
		}

		void MainWindow::loadParameters(QString fileName) {
			QFile file(fileName);
			if (!file.open(QFile::ReadOnly | QFile::Text)) {
				QMessageBox::warning(this, tr("Fragmentarium"),
					tr("Cannot read file %1:\n%2.")
					.arg(fileName)
					.arg(file.errorString()));
				return;
			}

			QTextStream in(&file);
			QString settings = in.readAll();	
			variableEditor->setSettings(settings);
			statusBar()->showMessage(tr("Settings loaded from file"), 2000);
		}

		void MainWindow::loadParameters() {
			QString filter = "Fragment Parameters (*.fragparams);;All Files (*.*)";
			QString fileName = QFileDialog::getOpenFileName(this, tr("Load"), "", filter);
			if (fileName.isEmpty())
				return;

			loadParameters(fileName);
		}



		void MainWindow::createToolBars()
		{
			fileToolBar = addToolBar(tr("File Toolbar"));
			fileToolBar->addAction(newAction);
			fileToolBar->addAction(openAction);
			fileToolBar->addAction(saveAction);

			editToolBar = addToolBar(tr("Edit Toolbar"));
			editToolBar->addAction(cutAction);
			editToolBar->addAction(copyAction);
			editToolBar->addAction(pasteAction);

			renderToolBar = addToolBar(tr("Render Toolbar"));
			renderToolBar->addAction(renderAction);
			buildLabel = new QLabel("Build    ", this);
			renderToolBar->addWidget(buildLabel);

			renderModeToolBar = addToolBar(tr("Rendering Mode"));

			renderModeToolBar->addWidget(new QLabel("Render mode:", this));

			
			autoRefreshButton = new QPushButton( "Auto",renderModeToolBar);
			autoRefreshButton->setCheckable(true);
			autoRefreshButton->setChecked(true);
			manualRefreshButton = new QPushButton("Manual",renderModeToolBar);
			manualRefreshButton->setCheckable(true);
			continousRefreshButton = new QPushButton( "Continuous",renderModeToolBar);
			continousRefreshButton->setCheckable(true);
			animationButton = new QPushButton("Animation", renderModeToolBar );
			animationButton->setCheckable(true);
			QButtonGroup* bg =new QButtonGroup(renderModeToolBar);
			bg->addButton(autoRefreshButton);
			bg->addButton(manualRefreshButton);
			bg->addButton(continousRefreshButton);
			bg->addButton(animationButton);
			
			connect(autoRefreshButton, SIGNAL(clicked()), this, SLOT(renderModeChanged()));
			connect(manualRefreshButton, SIGNAL(clicked()), this, SLOT(renderModeChanged()));
			connect(continousRefreshButton, SIGNAL(clicked()), this, SLOT(renderModeChanged()));
			connect(animationButton, SIGNAL(clicked()), this, SLOT(renderModeChanged()));
			
			renderModeToolBar->addWidget(autoRefreshButton);
			renderModeToolBar->addWidget(manualRefreshButton);
			renderModeToolBar->addWidget(continousRefreshButton);
			renderModeToolBar->addWidget(animationButton);

			/*
			renderCombo->addItem("Automatic");
			renderCombo->addItem("Manual");
			renderCombo->addItem("Continuous");
			renderCombo->addItem("Animation");
			//renderCombo->addItem("Custom Resolution");
			connect(renderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(renderModeChanged(int)));
			renderModeToolBar->addWidget(renderCombo);
			*/
			
			renderButton = new QPushButton(renderModeToolBar);
			renderButton->setText("");
			// renderButton->setShortcut(Qt::Key_F6); doesn't work?
			connect(renderButton, SIGNAL(clicked()), this, SLOT(callRedraw()));
			renderModeToolBar->addWidget(renderButton);

			viewLabel = new QLabel("Tile Preview (off)", renderModeToolBar);
			viewSlider = new QSlider(Qt::Horizontal,renderModeToolBar);
			viewSlider->setTickInterval(1);
			viewSlider->setMinimum(0);
			viewSlider->setMaximum(12);
			viewSlider->setTickPosition(QSlider::TicksBelow);
			viewSlider->setMaximumWidth(100);
			connect(viewSlider, SIGNAL(sliderReleased()), this, SLOT(viewSliderChanged()));
			renderModeToolBar->addWidget(viewLabel);
			renderModeToolBar->addWidget(viewSlider);
			viewSliderChanged();

			previewLabel = new QLabel("Preview (off)", renderModeToolBar);
			previewSlider = new QSlider(Qt::Horizontal,renderModeToolBar);
			previewSlider->setTickInterval(1);
			previewSlider->setMinimum(0);
			previewSlider->setMaximum(4);
			previewSlider->setTickPosition(QSlider::TicksBelow);
			previewSlider->setMaximumWidth(100);
			connect(previewSlider, SIGNAL(sliderReleased()), this, SLOT(previewSliderChanged()));
			renderModeToolBar->addWidget(previewLabel);
			renderModeToolBar->addWidget(previewSlider);

			frameLabel = new QLabel("Subframe 1. Max: ", renderModeToolBar);
			renderModeToolBar->addWidget(frameLabel);
			frameSpinBox = new QSpinBox(renderModeToolBar);
			frameSpinBox->setRange(0,1000);
			frameSpinBox->setValue(10);
			frameSpinBox->setSingleStep(5);

			connect(frameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(maxSubSamplesChanged(int)));
			
			renderModeToolBar->addWidget(frameSpinBox);
			
		}

		void MainWindow::maxSubSamplesChanged(int i) {
			engine->setMaxSubFrames(i);
		}

		void MainWindow::viewSliderChanged() {
			int v = viewSlider->value();
			if (v>0) {
				viewLabel->setText(QString("  Tile Preview (%1x)").arg(abs(v+1)));
			} else {
				viewLabel->setText(QString("  Tile Preview (off)"));
			}
			engine->setViewFactor(v);
		}

		void MainWindow::previewSliderChanged() {
			int v = previewSlider->value();
			if (v>0) {
				previewLabel->setText(QString("  Preview (%1x)").arg(abs(v+1)));
			} else {
				previewLabel->setText(QString("  Preview (off)"));
			}
			engine->setPreviewFactor(v);
		}


		void MainWindow::setSubFrameMax(int i) {
			frameSpinBox->setValue(i);
			engine->setMaxSubFrames(i);
		}

		void MainWindow::renderModeChanged() {
			
			QObject* o = QObject::sender();
			if (o == 0 || o == autoRefreshButton) {
				INFO("Automatic screen updates. Every time a parameter or camera changes, an update is triggered.");
				engine->setMaxSubFrames(0);
			} else if (o == manualRefreshButton) {
				INFO("Manual screen updates. Press 'update' to refresh the screen.");
				engine->setMaxSubFrames(0);
			} else if (o == continousRefreshButton) {
				INFO("Continuous screen updates. Updates at a fixed interval.");
				engine->setMaxSubFrames(frameSpinBox->value());
			}  else if (o == animationButton) {
				INFO("Animation mode. Use controller to jump in time.");
				engine->setMaxSubFrames(0);
			}
			if (o == animationButton) {
				animationController->show();
				engine->setAnimationSettings(((AnimationController*)animationController)->getAnimationSettings());
				animationController->resize(animationController->width(), animationController->minimumHeight());
			} else {
				engine->setAnimationSettings(0);
				animationController->hide();
			}
			renderButton->setEnabled(o!=autoRefreshButton && o!=animationButton);
			renderButton->setText( (o==continousRefreshButton) ? "Reset Time" : "Update (F6)");
			engine->setContinuous(o==continousRefreshButton);
			engine->setDisableRedraw(o == manualRefreshButton);

			if (o!=continousRefreshButton) setFPS(0);
		}

		void MainWindow::setSubFrameDisplay(int i) {
			frameLabel->setText(QString("Subframe %1. Max: ").arg(i));
		}

		void MainWindow::callRedraw() {
			bool state = engine->isRedrawDisabled();
			engine->setDisableRedraw(false);

			if (continousRefreshButton->isChecked()) {
				engine->resetTime();
			} else {
				engine->requireRedraw(true);
			}
			engine->setDisableRedraw(state);

		}

		void MainWindow::disableAllExcept(QWidget* w) {
			disabledWidgets.clear();
			disabledWidgets = findChildren<QWidget *>("");
			while (w) { disabledWidgets.removeAll(w); w=w->parentWidget(); }
			foreach (QWidget* w, disabledWidgets) w->setEnabled(false);
			qApp->processEvents();
		}

		void MainWindow::enableAll() {
			foreach (QWidget* w, disabledWidgets) w->setEnabled(true);
		}


		void MainWindow::createStatusBar()
		{
			statusBar()->showMessage(tr("Ready"));
		}

		void MainWindow::readSettings()
		{
			QSettings settings;
			QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
			QSize size = settings.value("size", QSize(1024, 800)).toSize();
			move(pos);
			resize(size);
		}

		void MainWindow::writeSettings()
		{
			QSettings settings;
			settings.setValue("pos", pos());
			settings.setValue("size", size());
		}


		void MainWindow::openFile()
		{
			QAction *action = qobject_cast<QAction *>(sender());
			if (action) {
				loadFile(action->data().toString());
			} else {
				WARNING("No data!");
			}
		}

		void MainWindow::loadFile(const QString &fileName)
		{
			insertTabPage(fileName);
			if (render()) {
				bool requiresRecompile = variableEditor->setDefault();
				if (requiresRecompile || rebuildRequired) {
					INFO("The 'default' settings needs to update the locking. You should recompile.");
					render();
				} else {
					//INFO("No recompile");
				}
			}
		}

		bool MainWindow::saveFile(const QString &fileName)
		{
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return false; } 

			QFile file(fileName);
			if (!file.open(QFile::WriteOnly | QFile::Text)) {
				QMessageBox::warning(this, tr("Fragmentarium"),
					tr("Cannot write file %1:\n%2.")
					.arg(fileName)
					.arg(file.errorString()));
				return false;
			}

			QTextStream out(&file);
			QApplication::setOverrideCursor(Qt::WaitCursor);
			out << getTextEdit()->toPlainText();
			QApplication::restoreOverrideCursor();

			tabInfo[tabBar->currentIndex()].hasBeenSavedOnce = true;
			tabInfo[tabBar->currentIndex()].unsaved = false;
			tabInfo[tabBar->currentIndex()].filename = fileName;
			tabChanged(tabBar->currentIndex()); // to update displayed name;

			statusBar()->showMessage(tr("File saved"), 2000);
			setRecentFile(fileName);

			return true;
		}



		QString MainWindow::strippedName(const QString &fullFileName)
		{
			return QFileInfo(fullFileName).fileName();
		}

		void MainWindow::showDebug() {
			logger->getListWidget()->clear();
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; } 
			INFO("Showing preprocessed output in new tab");
			QString inputText = getTextEdit()->toPlainText();
			QString filename = tabInfo[tabBar->currentIndex()].filename;
			QSettings settings;
			bool moveMain = settings.value("moveMain", true).toBool();
			bool doublify = settings.value("doublify", false).toBool();
			QStringList includePaths = settings.value("includePaths", "Examples/Include;").toString().split(";", QString::SkipEmptyParts);
			fileManager.setIncludePaths(includePaths);
			Preprocessor p(&fileManager);	
			try {
				FragmentSource fs = p.parse(inputText,filename,moveMain,doublify);
				QString prepend =  "#define highp\n"
					"#define mediump\n"
					"#define lowp\n";
				insertTabPage("")->setText(prepend+fs.getText());
			} catch (Exception& e) {
				WARNING(e.getMessage());
			}
		}

		void MainWindow::highlightBuildButton(bool value) {
			QWidget* w = buildLabel->parentWidget();
			if (value) {
				QPalette pal = w->palette();
				pal.setColor(w->backgroundRole(), Qt::yellow);
				w->setPalette(pal);
				w->setAutoFillBackground(true);
			} else {
				w->setPalette(QApplication::palette(w));
				w->setAutoFillBackground(false);
			}
			rebuildRequired = value;
		}

		bool MainWindow::render() {
			logger->getListWidget()->clear();

			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return false; } 
			QString inputText = getTextEdit()->toPlainText();
			if (inputText.startsWith("#donotrun")) { INFO("Not a runnable fragment."); return false; }
			QString filename = tabInfo[tabBar->currentIndex()].filename;
			QSettings settings;
			bool moveMain = settings.value("moveMain", true).toBool();
			QStringList includePaths = settings.value("includePaths", "Examples/Include;").toString().split(";", QString::SkipEmptyParts);
			QTime start = QTime::currentTime();
			fileManager.setIncludePaths(includePaths);
			fileManager.setOriginalFileName(filename);
					
			Preprocessor p(&fileManager);
			bool doublify = settings.value("doublify", false).toBool();

			try {
				FragmentSource fs = p.parse(inputText,filename,moveMain, doublify);
				bool showGUI = false;
				highlightBuildButton(false);
				variableEditor->updateFromFragmentSource(&fs, &showGUI);
				variableEditor->substituteLockedVariables(&fs);
				variableEditor->updateTextures(&fs, &fileManager);
				editorDockWidget->setHidden(!showGUI);
				engine->setFragmentShader(fs);
				variableEditor->updateCamera(engine->getCameraControl());	
				engine->requireRedraw(true);
				engine->resetTime();
			} catch (Exception& e) {
				WARNING(e.getMessage());
			}	
			int ms = start.msecsTo(QTime::currentTime());
			if (engine->hasShader()) {
				INFO(QString("Compiled script in %1 ms.").arg(ms));
			} else {
				WARNING(QString("Failed to compile script (%1 ms).").arg(ms));
			}
			return true;
		}

		namespace {
			// Returns the first valid directory.
			QString findDirectory(QStringList guesses) {
				QStringList invalid;
				for (int i = 0; i < guesses.size(); i++) {
					if (QFile::exists(guesses[i])) return guesses[i];
					invalid.append(QFileInfo(guesses[i]).absoluteFilePath());
				}

				// not found.
				WARNING("Could not locate directory in: " + invalid.join(",") + ".");
				return "[not found]";
			}
		}

		// Mac needs to step two directies up, when debugging in XCode...
		QString MainWindow::getExamplesDir() {
			QStringList examplesDir;
			examplesDir << "Examples" << "../../Examples";
			return findDirectory(examplesDir);
		}

		QString MainWindow::getMiscDir() {
			QStringList miscDir;
			miscDir << "Misc" << "../../Misc";
			return findDirectory(miscDir);
		}

		QString MainWindow::getTemplateDir() {
			return getMiscDir();
		}


		QTextEdit* MainWindow::getTextEdit() {
			return (stackedTextEdits->currentWidget() ? (QTextEdit*)stackedTextEdits->currentWidget() : 0);
		}

		void MainWindow::cursorPositionChanged() {
			if (!this->getTextEdit()) return;
			int pos = this->getTextEdit()->textCursor().position();
			int blockNumber = this->getTextEdit()->textCursor().blockNumber();

			// Do reverse look up...
			FragmentSource* fs = engine->getFragmentSource();
			QString x;
			QStringList ex;
			QString filename = tabInfo[tabBar->currentIndex()].filename;

			for (int i = 0; i < fs->lines.count(); i++) {
				// fs->sourceFiles[fs->sourceFile[i]]->fileName()
				if (fs->lines[i] == blockNumber && 
					QString::compare(filename,fs->sourceFileNames[fs->sourceFile[i]], Qt::CaseInsensitive)==0
				) ex.append(QString::number(i+4));
			}
			if (ex.count()) {
				x = " Line in preprocessed script: " + ex.join(",");
			} else {
				x = " (Not part of current script) ";
			}

			statusBar()->showMessage(QString("Position: %1, Line: %2.").arg(pos).arg(blockNumber+1)+x, 5000);
		}

		QTextEdit* MainWindow::insertTabPage(QString filename) {
			QTextEdit* textEdit = new TextEdit(this);
			connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));

			textEdit->setLineWrapMode(QTextEdit::NoWrap);
			textEdit->setTabStopWidth(20);
			new FragmentHighlighter(textEdit);

			QString s = QString("// Write fragment code here...\r\n");
			textEdit->setText(s);

			bool loadingSucceded = false;
			if (!filename.isEmpty()) {
				INFO(QString("Loading file: %1").arg(filename));
				QFile file(filename);
				if (!file.open(QFile::ReadOnly | QFile::Text)) {
					textEdit->setPlainText(QString("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString()));
				} else {
					QTextStream in(&file);
					QApplication::setOverrideCursor(Qt::WaitCursor);
					textEdit->setPlainText(in.readAll());
					QApplication::restoreOverrideCursor();
					statusBar()->showMessage(QString("Loaded file: %1").arg(filename), 2000);
					loadingSucceded = true;
				}
			}


			QString displayName = filename;
			if (displayName.isEmpty()) {
				// Find a new name
				displayName = "Unnamed";
				QString suggestedName = displayName;

				bool unique = false;
				int counter = 1;
				while (!unique) {
					unique = true;
					for (int i = 0; i < tabInfo.size(); i++) {
						if (tabInfo[i].filename == suggestedName) {
							//INFO("equal");
							unique = false;
							break;
						}	
					}
					if (!unique) suggestedName = displayName + " " + QString::number(counter++);
				}
				displayName = suggestedName;
			}

			stackedTextEdits->addWidget(textEdit);

			if (loadingSucceded) {
				tabInfo.append(TabInfo(displayName, textEdit, false, true));
				setRecentFile(filename);
				setWatchingTarget(filename);

			} else {
				tabInfo.append(TabInfo(displayName, textEdit, true));
			}

			QString tabTitle = QString("%1%3").arg(strippedName(displayName)).arg(!loadingSucceded? "*" : "");
			tabBar->setCurrentIndex(tabBar->addTab(strippedName(tabTitle)));

			connect(textEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
			return textEdit;
		}

		void MainWindow::resetCamera(bool fullReset) {
			engine->resetCamera(fullReset);
		}

		void MainWindow::tabChanged(int index) {
			if (index > tabInfo.size()) return;
			if (index < 0) return;

			TabInfo t = tabInfo[index];
			QString tabTitle = QString("%1%3").arg(strippedName(t.filename)).arg(t.unsaved ? "*" : "");
			setWindowTitle(QString("%1 - %2").arg(tabTitle).arg("Fragmentarium"));
			stackedTextEdits->setCurrentWidget(t.textEdit);
			tabBar->setTabText(tabBar->currentIndex(), tabTitle);
			setWatchingTarget(t.filename);
		}

		void MainWindow::closeTab() {
			int index = tabBar->currentIndex();
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; } 
			closeTab(index);
		}

		void MainWindow::closeTab(int index) {		
			TabInfo t = tabInfo[index];
			if (t.unsaved) {
				int answer = QMessageBox::warning(this, QString("Unsaved changes"), "Close this tab without saving changes?", "OK", "Cancel");
				if (answer == 1) return;
			}

			tabInfo.remove(index);
			tabBar->removeTab(index);

			stackedTextEdits->removeWidget(t.textEdit);
			delete(t.textEdit); // ?
		}

		void MainWindow::launchSfHome() {
			INFO("Launching web browser...");
			bool s = QDesktopServices::openUrl(QUrl("http://syntopia.github.com/Fragmentarium/"));
			if (!s) WARNING("Failed to open browser...");
		}

		void MainWindow::launchGLSLSpecs() {
			INFO("Launching web browser...");
			bool s = QDesktopServices::openUrl(QUrl("http://www.opengl.org/registry/"));
			if (!s) WARNING("Failed to open browser...");
		}

		void MainWindow::launchIntro() {
			INFO("Launching web browser...");
			bool s = QDesktopServices::openUrl(QUrl("http://blog.hvidtfeldts.net/index.php/2011/06/distance-estimated-3d-fractals-part-i/"));
			if (!s) WARNING("Failed to open browser...");
		}

		void MainWindow::launchFAQ() {
			INFO("Launching web browser...");
			bool s = QDesktopServices::openUrl(QUrl("http://blog.hvidtfeldts.net/index.php/2011/12/fragmentarium-faq/"));
			if (!s) WARNING("Failed to open browser...");
		}


		void MainWindow::launchReferenceHome() {
			INFO("Launching web browser...");
			bool s = QDesktopServices::openUrl(QUrl("http://syntopia.github.com/Fragmentarium/"));
			if (!s) WARNING("Failed to open browser...");
		}

		void MainWindow::launchGallery() {
			INFO("Launching web browser...");
			bool s = QDesktopServices::openUrl(QUrl("http://flickr.com/groups/fragmentarium/"));
			if (!s) WARNING("Failed to open browser...");
		}

		void MainWindow::makeScreenshot() {
			saveImage(engine->grabFrameBuffer());
		}

		void MainWindow::saveImage(QImage image) {
			QString filename = SyntopiaCore::Misc::GetImageFileName(this, "Save screenshot as:");
			if (filename.isEmpty()) return;
			bool succes = image.save(filename);
			if (succes) {
				INFO("Saved screenshot as: " + filename);
			} else {
				WARNING("Save failed! Filename: " + filename);
			}
		}



		void MainWindow::copy() {
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; } 
			getTextEdit()->copy();
		}


		void MainWindow::cut() {
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; } 
			getTextEdit()->cut();
		}

		void MainWindow::paste() {
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; } 
			getTextEdit()->paste();
		}

		void MainWindow::dragEnterEvent(QDragEnterEvent *ev)
		{
			if (ev->mimeData()->hasUrls()) {
				ev->acceptProposedAction();
			} else {
				INFO("Cannot accept MIME object: " + ev->mimeData()->formats().join(" - "));
			}
		}

		void MainWindow::dropEvent(QDropEvent *ev) {
			if (ev->mimeData()->hasUrls()) {
				QList<QUrl> urls = ev->mimeData()->urls();
				for (int i = 0; i < urls.size() ; i++) {
					QString file = urls[i].toLocalFile();
					INFO("Loading: " + file);
					if (file.toLower().endsWith(".fragparams")) {
						loadParameters(file);
					} else {
						loadFile(file);
					}
				}
			} else {
				INFO("Cannot accept MIME object: " + ev->mimeData()->formats().join(" - "));
			}
		}

		void MainWindow::setRecentFile(const QString &fileName)
		{
			QSettings settings;

			QStringList files = settings.value("recentFileList").toStringList();
			files.removeAll(fileName);
			files.prepend(fileName);
			while (files.size() > MaxRecentFiles) files.removeLast();

			settings.setValue("recentFileList", files);

			int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

			for (int i = 0; i < numRecentFiles; ++i) {
				QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
				recentFileActions[i]->setText(text);
				QString absPath = QFileInfo(files[i]).absoluteFilePath();
				recentFileActions[i]->setData(absPath);
				recentFileActions[i]->setVisible(true);
			}

			for (int j = numRecentFiles; j < MaxRecentFiles; ++j) recentFileActions[j]->setVisible(false);

			recentFileSeparator->setVisible(numRecentFiles > 0);
		}

		void MainWindow::setSplashWidget(QWidget* w) {
			splashWidget = w;
			QTimer::singleShot(3000, this, SLOT(removeSplash()));
		}

		void MainWindow::removeSplash() {
			splashWidget->close();
		}

		void MainWindow::insertText() {
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; } 

			QString text = ((QAction*)sender())->text();
			getTextEdit()->insertPlainText(text.section("//",0,0)); // strip comments
		}

		void MainWindow::preferences() {
			PreferencesDialog pd(this);
			pd.exec();
			getEngine()->updateRefreshRate();
		}

		void MainWindow::indent() {
			if (tabBar->currentIndex() == -1) { WARNING("No open tab"); return; } 

			int hValue =  getTextEdit()->horizontalScrollBar()->value();
			int vValue =  getTextEdit()->verticalScrollBar()->value();
			int cPos = getTextEdit()->textCursor().position();
			QStringList l = getTextEdit()->toPlainText().split("\n");
			QStringList out;
			int indent = 0;
			foreach (QString s, l) {
				int offset = s.trimmed().startsWith("}") ? -1 : 0;
				QString newString = s.trimmed();
				for (int i = 0; i < indent+offset; i++) newString.push_front("\t");
				out.append(newString);
				indent = indent + s.count("{")+s.count("(") - s.count("}") -s.count(")");
			}
			getTextEdit()->setText(out.join("\n"));

			getTextEdit()->horizontalScrollBar()->setValue(hValue);
			getTextEdit()->verticalScrollBar()->setValue(vValue);
			QTextCursor tc = getTextEdit()->textCursor();
			tc.setPosition(cPos);
			getTextEdit()->setTextCursor(tc);

		}

		void MainWindow::setFPS(float fps) {

			if (!continousRefreshButton->isChecked()) {
				fpsLabel->setText("FPS: n.a.");
				return;
			}

			if (fps>0) {
				fpsLabel->setText("FPS: " + QString::number(fps, 'f' ,1) + " (" +  QString::number(1.0/fps, 'f' ,1) + "s)");
			}
		}

		void MainWindow::setWatchingTarget(const QString& path)
		{
			if (!watchingPath.isEmpty()) {
				fileWatcher.removePath(watchingPath);
			}
			else {
				reloadAction = new QAction(this);
				connect(reloadAction, SIGNAL(triggered()), this, SLOT(onFileReload()));
				connect(&fileWatcher, SIGNAL(fileChanged(const QString&)), this, SLOT(onFileChange(const QString&)));
			}
			fileWatcher.addPath(path);
			watchingPath = path;
		}

		void MainWindow::onFileChange(const QString& path)
		{
			reloadAction->activate(QAction::Trigger);
		}

		void MainWindow::onFileReload()
		{
			if (!tabInfo[tabBar->currentIndex()].unsaved) {
				QTextEdit* textEdit = tabInfo[tabBar->currentIndex()].textEdit;
				QString filename = watchingPath;

				INFO(QString("Loading file: %1").arg(filename));
				QFile file(filename);
				if (!file.open(QFile::ReadOnly | QFile::Text)) {
					textEdit->setPlainText(QString("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString()));
				} else {
					QTextStream in(&file);
					QApplication::setOverrideCursor(Qt::WaitCursor);
					textEdit->setPlainText(in.readAll());
					QApplication::restoreOverrideCursor();
					statusBar()->showMessage(QString("Loaded file: %1").arg(filename), 2000);
				}

				tabInfo[tabBar->currentIndex()].unsaved = false;
				tabChanged(tabBar->currentIndex());
			}

			render();
		}
	}
}

