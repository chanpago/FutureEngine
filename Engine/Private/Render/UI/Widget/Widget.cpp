#include "pch.h"
#include "Render/UI/Widget/Widget.h"

IMPLEMENT_ABSTRACT_CLASS(UWidget, UObject)


UWidget::UWidget(const FString& InName) : UObject(InName)
{
}
