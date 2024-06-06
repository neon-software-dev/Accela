/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "TransformComponentWidget.h"

#include "../ViewModel/MainWindowVM.h"

#include <QLabel>
#include <QLineEdit>
#include <QBoxLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QFormLayout>

namespace Accela
{

TransformComponentWidget::TransformComponentWidget(std::shared_ptr<MainWindowVM> mainVM, QWidget *pParent)
    : QWidget(pParent)
    , m_mainVM(std::move(mainVM))
{
    InitUI();
    BindVM();
}

TransformComponentWidget::~TransformComponentWidget()
{
    m_pPositionXSpinBox = nullptr;
    m_pPositionYSpinBox = nullptr;
    m_pPositionZSpinBox = nullptr;
}

static inline QDoubleSpinBox* CreatePositionSpinBox()
{
    auto pDoubleSpinBox = new QDoubleSpinBox();
    pDoubleSpinBox->setValue(0.0f);
    pDoubleSpinBox->setSingleStep(0.1f);
    pDoubleSpinBox->setSuffix(" m");
    pDoubleSpinBox->setRange(-std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    return pDoubleSpinBox;
}

static inline QDoubleSpinBox* CreateRotationSpinBox()
{
    auto pDoubleSpinBox = new QDoubleSpinBox();
    pDoubleSpinBox->setValue(0.0f);
    pDoubleSpinBox->setSingleStep(0.1f);
    pDoubleSpinBox->setSuffix(" deg");
    pDoubleSpinBox->setRange(-360.0f, 360.0f);

    return pDoubleSpinBox;
}

static inline QDoubleSpinBox* CreateScaleSpinBox()
{
    auto pDoubleSpinBox = new QDoubleSpinBox();
    pDoubleSpinBox->setValue(1.0f);
    pDoubleSpinBox->setSingleStep(0.1f);
    pDoubleSpinBox->setSuffix("%");
    pDoubleSpinBox->setRange(-std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    return pDoubleSpinBox;
}

void TransformComponentWidget::InitUI()
{
    auto pGroupBox = new QGroupBox(tr("Transform Component"), this);
    auto pGroupBoxLayout = new QVBoxLayout(pGroupBox);

    auto pPositionFormLayout = new QFormLayout();
    pPositionFormLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pPositionFormLayout->setLabelAlignment(Qt::AlignLeft);

    m_pPositionXSpinBox = CreatePositionSpinBox();
    connect(m_pPositionXSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnPositionXSpinValueChanged);
    pPositionFormLayout->addRow(tr("Pos X"), m_pPositionXSpinBox);

    m_pPositionYSpinBox = CreatePositionSpinBox();
    connect(m_pPositionYSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnPositionYSpinValueChanged);
    pPositionFormLayout->addRow(tr("Pos Y"), m_pPositionYSpinBox);

    m_pPositionZSpinBox = CreatePositionSpinBox();
    connect(m_pPositionZSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnPositionZSpinValueChanged);
    pPositionFormLayout->addRow(tr("Pos Z"), m_pPositionZSpinBox);

    auto pRotationFormLayout = new QFormLayout();
    pRotationFormLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pRotationFormLayout->setLabelAlignment(Qt::AlignLeft);
    pRotationFormLayout->addRow(tr("Rot X"), CreateRotationSpinBox());
    pRotationFormLayout->addRow(tr("Rot Y"), CreateRotationSpinBox());
    pRotationFormLayout->addRow(tr("Rot Z"), CreateRotationSpinBox());

    auto pScaleFormLayout = new QFormLayout();
    pScaleFormLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pScaleFormLayout->setLabelAlignment(Qt::AlignLeft);
    pScaleFormLayout->addRow(tr("Scale X"), CreateScaleSpinBox());
    pScaleFormLayout->addRow(tr("Scale Y"), CreateScaleSpinBox());
    pScaleFormLayout->addRow(tr("Scale Z"), CreateScaleSpinBox());

    pGroupBoxLayout->addLayout(pPositionFormLayout);
    pGroupBoxLayout->addLayout(pRotationFormLayout);
    pGroupBoxLayout->addLayout(pScaleFormLayout);

    auto pLayout = new QVBoxLayout(this);
    pLayout->addWidget(pGroupBox);

    //
    // Initial contents update
    //
    UpdateFieldContents();
}

void TransformComponentWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnComponentInvalidated, this, &TransformComponentWidget::VM_OnComponentInvalidated);
}

void TransformComponentWidget::UI_OnPositionXSpinValueChanged(double d)
{
    // Ignore value changes from syncing field contents
    if (m_updatingFieldContents) { return; }

    auto transformComponent = m_mainVM->GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (transformComponent == nullptr)
    {
        assert(false);
        return;
    }

    auto position = (*transformComponent)->component.GetPosition();
    position.x = (float)d;
    (*transformComponent)->component.SetPosition(position);

    m_mainVM->OnComponentInvalidated(*transformComponent);
}

void TransformComponentWidget::UI_OnPositionYSpinValueChanged(double d)
{
    // Ignore value changes from syncing field contents
    if (m_updatingFieldContents) { return; }

    auto transformComponent = m_mainVM->GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (transformComponent == nullptr)
    {
        assert(false);
        return;
    }

    auto position = (*transformComponent)->component.GetPosition();
    position.y = (float)d;
    (*transformComponent)->component.SetPosition(position);

    m_mainVM->OnComponentInvalidated(*transformComponent);
}

void TransformComponentWidget::UI_OnPositionZSpinValueChanged(double d)
{
    // Ignore value changes from syncing field contents
    if (m_updatingFieldContents) { return; }

    auto transformComponent = m_mainVM->GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (!transformComponent)
    {
        assert(false);
        return;
    }

    auto position = (*transformComponent)->component.GetPosition();
    position.z = (float)d;
    (*transformComponent)->component.SetPosition(position);

    m_mainVM->OnComponentInvalidated(*transformComponent);
}

void TransformComponentWidget::VM_OnComponentInvalidated(const Engine::CEntity::Ptr&, const Engine::Component::Ptr& component)
{
    if (component->GetType() != Engine::Component::Type::Transform)
    {
        return;
    }

    UpdateFieldContents();
}

void TransformComponentWidget::UpdateFieldContents()
{
    m_updatingFieldContents = true;

    //
    // Clear State
    //

    //
    // Update State
    //
    const auto& entity = m_mainVM->GetModel().entity;
    if (!entity)
    {
        m_updatingFieldContents = false;
        return;
    }

    auto transformComponent = m_mainVM->GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (!transformComponent)
    {
        assert(false);
        m_updatingFieldContents = false;
        return;
    }

    m_pPositionXSpinBox->setValue((*transformComponent)->component.GetPosition().x);
    m_pPositionYSpinBox->setValue((*transformComponent)->component.GetPosition().y);
    m_pPositionZSpinBox->setValue((*transformComponent)->component.GetPosition().z);

    m_updatingFieldContents = false;
}

}
