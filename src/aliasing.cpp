#include "aliasing.h"
#include "userpanel.h"

Aliasing::Aliasing(QObject *parent)
    : QObject{parent}
{
    inference = new QTableView();
    inference_model = new QStandardItemModel();

    apply_btn_grp = new QButtonGroup(this);
    for (int i = 0; i < 31; i++) {
        QPushButton *btn = new QPushButton("APPLY");
        apply_btn_grp->addButton(btn, i);
    }
    connect(apply_btn_grp, static_cast<void (QButtonGroup::*)(int id)>(&QButtonGroup::buttonClicked), this,
            [this](int id){ emit delay_set_selected(current_selection = id); });

    inference->setWindowTitle("Aliasing");
    inference->resize(600, 800);
    inference->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    inference->horizontalHeader()->setHighlightSections(false);
    inference->verticalHeader()->setHighlightSections(false);
    inference->setModel(inference_model);
    inference->setAlternatingRowColors(true);
}

void Aliasing::update_distance(float distance)
{
//    inference_model->clear();
    original_distance = distance;
    inference_model->setHorizontalHeaderLabels({"PRF\n(kHz)", "T\n(μs)", "result dist\n(m)", "result delay\n(μs)", "# T\n(1)", ""});
    float period, result_dist, result_delay, num_period;
    for (int i = 0 ; i < 30; i++) {
        QStandardItem *prf_item = new QStandardItem(QString::asprintf("%02d", i + 1));
        prf_item->setData(i + 1);
        prf_item->setEditable(false);
        prf_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        inference_model->setItem(i, 0, prf_item);

        period = 1000. / (i + 1);
        QStandardItem *period_item = new QStandardItem(QString::asprintf("%.3f", period));
        period_item->setData(period);
        period_item->setEditable(false);
        period_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        inference_model->setItem(i, 1, period_item);

        num_period = std::floor(distance / (150 * period));
        QStandardItem *num_period_item = new QStandardItem(QString::asprintf("%d", (int)num_period));
        num_period_item->setData(num_period);
        num_period_item->setEditable(false);
        num_period_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        inference_model->setItem(i, 4, num_period_item);

        result_dist = distance - num_period * 150 * period;
        QStandardItem *result_dist_item = new QStandardItem(QString::asprintf("%.3f", result_dist));
        result_dist_item->setData(result_dist);
        result_dist_item->setEditable(false);
        result_dist_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        inference_model->setItem(i, 2, result_dist_item);

        result_delay = result_dist / 150;
        QStandardItem *result_delay_item = new QStandardItem(QString::asprintf("%.3f", result_delay));
        result_delay_item->setData(result_delay);
        result_delay_item->setEditable(false);
        result_delay_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        inference_model->setItem(i, 3, result_delay_item);

        inference->setIndexWidget(inference_model->index(i, 5), apply_btn_grp->button(i));
    }
}

void Aliasing::show_ui()
{
    inference->show();
    inference->raise();
}

AliasingData Aliasing::retrieve_data(int id)
{
    return AliasingData {inference_model->item(id, 0)->data().toFloat(),
                         inference_model->item(id, 2)->data().toFloat(),
                         inference_model->item(id, 4)->data().toInt()};
}
