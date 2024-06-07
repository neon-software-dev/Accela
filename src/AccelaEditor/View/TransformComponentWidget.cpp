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
    : ComponentWidget(tr("Transform"), Engine::Component::Type::Transform, std::move(mainVM), pParent)
{
    InitUI(CreateComponentUI());
    BindVM();
}

TransformComponentWidget::~TransformComponentWidget()
{
    m_pPositionXSpinBox = nullptr;
    m_pPositionYSpinBox = nullptr;
    m_pPositionZSpinBox = nullptr;
    m_pRotationXSpinBox = nullptr;
    m_pRotationYSpinBox = nullptr;
    m_pRotationZSpinBox = nullptr;
    m_pScaleXSpinBox = nullptr;
    m_pScaleYSpinBox = nullptr;
    m_pScaleZSpinBox = nullptr;
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
    pDoubleSpinBox->setRange(0.0f, std::numeric_limits<float>::max());

    return pDoubleSpinBox;
}

void TransformComponentWidget::InitUI(QBoxLayout* pContentLayout)
{
    auto pPositionFormLayout = new QFormLayout();
    pPositionFormLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pPositionFormLayout->setLabelAlignment(Qt::AlignLeft);

    m_pPositionXSpinBox = CreatePositionSpinBox();
    connect(m_pPositionXSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnPositionSpinValueChanged);
    pPositionFormLayout->addRow(tr("Pos X"), m_pPositionXSpinBox);

    m_pPositionYSpinBox = CreatePositionSpinBox();
    connect(m_pPositionYSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnPositionSpinValueChanged);
    pPositionFormLayout->addRow(tr("Pos Y"), m_pPositionYSpinBox);

    m_pPositionZSpinBox = CreatePositionSpinBox();
    connect(m_pPositionZSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnPositionSpinValueChanged);
    pPositionFormLayout->addRow(tr("Pos Z"), m_pPositionZSpinBox);

    auto pRotationFormLayout = new QFormLayout();
    pRotationFormLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pRotationFormLayout->setLabelAlignment(Qt::AlignLeft);
    m_pRotationXSpinBox = CreateRotationSpinBox();
    connect(m_pRotationXSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnRotationSpinValueChanged);
    pRotationFormLayout->addRow(tr("Rot X"), m_pRotationXSpinBox);
    m_pRotationYSpinBox = CreateRotationSpinBox();
    connect(m_pRotationYSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnRotationSpinValueChanged);
    pRotationFormLayout->addRow(tr("Rot Y"), m_pRotationYSpinBox);
    m_pRotationZSpinBox = CreateRotationSpinBox();
    connect(m_pRotationZSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnRotationSpinValueChanged);
    pRotationFormLayout->addRow(tr("Rot Z"), m_pRotationZSpinBox);

    auto pScaleFormLayout = new QFormLayout();
    pScaleFormLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pScaleFormLayout->setLabelAlignment(Qt::AlignLeft);
    m_pScaleXSpinBox = CreateScaleSpinBox();
    connect(m_pScaleXSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnScaleSpinValueChanged);
    pScaleFormLayout->addRow(tr("Scale X"), m_pScaleXSpinBox);
    m_pScaleYSpinBox = CreateScaleSpinBox();
    connect(m_pScaleYSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnScaleSpinValueChanged);
    pScaleFormLayout->addRow(tr("Scale Y"), m_pScaleYSpinBox);
    m_pScaleZSpinBox = CreateScaleSpinBox();
    connect(m_pScaleZSpinBox, &QDoubleSpinBox::valueChanged, this, &TransformComponentWidget::UI_OnScaleSpinValueChanged);
    pScaleFormLayout->addRow(tr("Scale Z"), m_pScaleZSpinBox);

    pContentLayout->addLayout(pPositionFormLayout);
    pContentLayout->addLayout(pRotationFormLayout);
    pContentLayout->addLayout(pScaleFormLayout);

    //
    // Initial contents update
    //
    UpdateFieldContents();
}

void TransformComponentWidget::BindVM()
{
    connect(m_mainVM.get(), &MainWindowVM::VM_OnComponentInvalidated, this, &TransformComponentWidget::VM_OnComponentInvalidated);
}

void TransformComponentWidget::UI_OnPositionSpinValueChanged(double)
{
    // Ignore value changes from syncing field contents
    if (m_updatingFieldContents) { return; }

    UpdateComponentPositionValue();
}

void TransformComponentWidget::UI_OnRotationSpinValueChanged(double)
{
    // Ignore value changes from syncing field contents
    if (m_updatingFieldContents) { return; }

    UpdateComponentRotationValue();
}

void TransformComponentWidget::UI_OnScaleSpinValueChanged(double)
{
    // Ignore value changes from syncing field contents
    if (m_updatingFieldContents) { return; }

    UpdateComponentScaleValue();
}

void TransformComponentWidget::UpdateComponentPositionValue()
{
    auto transformComponent = m_mainVM->GetModel().GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (!transformComponent)
    {
        assert(false);
        return;
    }

    const glm::vec3 position(
        (float)m_pPositionXSpinBox->value(),
        (float)m_pPositionYSpinBox->value(),
        (float)m_pPositionZSpinBox->value()
    );

    (*transformComponent)->position = position;
    m_mainVM->OnComponentModified(*transformComponent);
}

void TransformComponentWidget::UpdateComponentRotationValue()
{
    auto transformComponent = m_mainVM->GetModel().GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (!transformComponent)
    {
        assert(false);
        return;
    }

    const glm::vec3 eulerRotation = {
        (float)m_pRotationXSpinBox->value(),
        (float)m_pRotationYSpinBox->value(),
        (float)m_pRotationZSpinBox->value()
    };

    (*transformComponent)->eulerRotation = eulerRotation;
    m_mainVM->OnComponentModified(*transformComponent);
}

void TransformComponentWidget::UpdateComponentScaleValue()
{
    auto transformComponent = m_mainVM->GetModel().GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (!transformComponent)
    {
        assert(false);
        return;
    }

    const glm::vec3 scale(
        (float)m_pScaleXSpinBox->value(),
        (float)m_pScaleYSpinBox->value(),
        (float)m_pScaleZSpinBox->value()
    );

    (*transformComponent)->scale = scale;
    m_mainVM->OnComponentModified(*transformComponent);
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

    auto transformComponent = m_mainVM->GetModel().GetEntityComponent<Engine::CTransformComponent>(Engine::Component::Type::Transform);
    if (!transformComponent)
    {
        assert(false);
        m_updatingFieldContents = false;
        return;
    }

    m_pPositionXSpinBox->setValue((*transformComponent)->position.x);
    m_pPositionYSpinBox->setValue((*transformComponent)->position.y);
    m_pPositionZSpinBox->setValue((*transformComponent)->position.z);

    m_pRotationXSpinBox->setValue((*transformComponent)->eulerRotation.x);
    m_pRotationYSpinBox->setValue((*transformComponent)->eulerRotation.y);
    m_pRotationZSpinBox->setValue((*transformComponent)->eulerRotation.z);

    m_pScaleXSpinBox->setValue((*transformComponent)->scale.x);
    m_pScaleYSpinBox->setValue((*transformComponent)->scale.y);
    m_pScaleZSpinBox->setValue((*transformComponent)->scale.z);

    m_updatingFieldContents = false;
}

}
