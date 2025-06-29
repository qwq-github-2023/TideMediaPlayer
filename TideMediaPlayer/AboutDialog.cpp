#include "AboutDialog.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDate>

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("About %1").arg(qApp->applicationName()));
    setFixedSize(500, 300);
    setupUI();
}

void AboutDialog::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);

    // 软件名称和版本
    QLabel* titleLabel = new QLabel(
        QString("<h1>%1</h1>"
            "<h3>Version %2</h3>")
        .arg(qApp->applicationName())
        .arg(qApp->applicationVersion()));
    titleLabel->setAlignment(Qt::AlignCenter);

    // 描述
    QLabel* descLabel = new QLabel(
        tr("An open-source software for browsing media purpose.<br>"
            "Hosted on GitHub."));
    descLabel->setWordWrap(true);

    // 许可证显示
    QTextBrowser* licenseBrowser = new QTextBrowser();
    licenseBrowser->setPlainText(getLicenseText());
    licenseBrowser->setOpenExternalLinks(true);

    // GitHub链接按钮
    QPushButton* githubBtn = new QPushButton(tr("Visit GitHub Repository"));
    connect(githubBtn, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/qwq-github-2023/TideMediaPlayer"));
        });

    // 布局
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    layout->addWidget(licenseBrowser);
    layout->addWidget(githubBtn);
}

QString AboutDialog::getLicenseText() {
    QFile licenseFile(":/LICENSE");
    if (licenseFile.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(licenseFile.readAll());
    }
    return tr("GPL-3.0 License\n"
        "Copyright © 2025 TideMediaPlayer Contributors.\n"
        "Permission is hereby granted...");
}