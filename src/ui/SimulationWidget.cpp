#include "ui/SimulationWidget.h"
#include "ui_SimulationWidget.h"
#include "simulation/URDFLoader.h"
#include "simulation/RobotRenderer.h"
#include <QFileDialog>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>

SimulationWidget::SimulationWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SimulationWidget)
    , m_renderer(nullptr)
    , m_urdfLoader(new URDFLoader())
{
    ui->setupUi(this);
    
    // 创建RobotRenderer并添加到renderContainer
    m_renderer = new RobotRenderer();
    QVBoxLayout* renderLayout = new QVBoxLayout(ui->renderContainer);
    renderLayout->setContentsMargins(0, 0, 0, 0);
    renderLayout->addWidget(m_renderer);
    
    // 扫描URDF文件
    scanURDFFiles();
    
    // 连接信号
    connect(ui->btnLoadURDF, &QPushButton::clicked, this, &SimulationWidget::onLoadClicked);
    
    qDebug() << "SimulationWidget initialized, renderer created";
}

SimulationWidget::~SimulationWidget()
{
    delete ui;
    delete m_urdfLoader;
}

void SimulationWidget::scanURDFFiles()
{
    ui->comboBoxURDF->clear();
    
    // 从资源文件中扫描所有 .urdf 文件
    QDirIterator it(":/models", QStringList() << "*.urdf", QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        
        // 显示文件名（相对路径）
        QString displayName = fileInfo.fileName();
        
        ui->comboBoxURDF->addItem(displayName, filePath);
        qDebug() << "Found URDF in resources:" << filePath;
    }
    
    // 如果找到了 URDF 文件，自动加载第一个
    if (ui->comboBoxURDF->count() > 0) {
        ui->comboBoxURDF->setCurrentIndex(0);
        qDebug() << "Auto-loading first URDF:" << ui->comboBoxURDF->currentText();
        
        // 自动加载第一个
        QString filePath = ui->comboBoxURDF->currentData().toString();
        if (!filePath.isEmpty()) {
            loadURDF(filePath);
        }
    } else {
        qWarning() << "No URDF files found in resources";
    }
}

void SimulationWidget::onLoadClicked()
{
    QString filePath = ui->comboBoxURDF->currentData().toString();
    qDebug() << "Loading URDF from:" << filePath;
    if (!filePath.isEmpty()) {
        loadURDF(filePath);
    }
}

void SimulationWidget::onBrowseClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, 
        "选择URDF文件", 
        ".", 
        "URDF Files (*.urdf);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        loadURDF(filePath);
    }
}

void SimulationWidget::loadURDF(const QString& filePath)
{
    qDebug() << "loadURDF called with:" << filePath;
    
    if (m_urdfLoader->loadURDF(filePath)) {
        qDebug() << "URDF loaded successfully";
        qDebug() << "Links:" << m_urdfLoader->getLinks().size();
        qDebug() << "Joints:" << m_urdfLoader->getJoints().size();
        
        m_renderer->setURDFData(m_urdfLoader->getLinks(), m_urdfLoader->getJoints());
        m_renderer->update(); // 强制重绘
        
        emit urdfLoaded(filePath);
    } else {
        qDebug() << "Failed to load URDF";
    }
}

void SimulationWidget::updateJointAngles(const QString& armName, const QVector<double>& joints)
{
    if (!m_renderer) {
        return;
    }
    
    // 按 URDF 中的关节名精确映射：<arm>_j1_joint ... <arm>_j7_joint
    // 这样左右臂各自独立，不会互相覆盖
    const QString prefix = (armName == "right") ? "right" : "left";
    
    const int n = qMin(joints.size(), 7);
    for (int i = 0; i < n; ++i) {
        const QString jointName = QString("%1_j%2_joint").arg(prefix).arg(i + 1);
        m_renderer->setJointAngleByName(jointName, joints[i]);
    }
    
    m_renderer->updateVisualization();
    m_renderer->update();
}
