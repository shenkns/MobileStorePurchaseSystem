// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Proxies/AndroidBillingHelper.h"

#include "Module/MobileStorePurchaseSystemModule.h"

#if PLATFORM_ANDROID

#include "Android/AndroidJavaEnv.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include <android_native_app_glue.h>

#endif

UAndroidBillingHelper* UAndroidBillingHelper::Get()
{
	const FMobileStorePurchaseSystemModule& BillingModule = FModuleManager::GetModuleChecked<FMobileStorePurchaseSystemModule>("MobileStorePurchaseSystem");
	
	return BillingModule.GetAndroidBillingHelper();
}

void UAndroidBillingHelper::RequestProducts(TArray<FString> ProductIDs)
{
#if PLATFORM_ANDROID
	LOG(LogMobileStorePurchaseSystem, "UE Billing Request Products")
	
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	if (!Env) return;
	
	jclass Class = FAndroidApplication::FindJavaClassGlobalRef("com/billing/unreal/UnrealBillingAndroid");
	if(!Class) return;

	auto Method = FJavaWrapper::FindStaticMethod(Env, Class, "queryProducts", "([Ljava/lang/String;)V", false);
	if(!Method) return;
	
	auto ProductIDArray = NewScopedJavaObject(Env, (jobjectArray)Env->NewObjectArray(ProductIDs.Num(), FJavaWrapper::JavaStringClass, NULL));
	if (!ProductIDArray) return;
	
	for (uint32 i = 0; i < ProductIDs.Num(); i++)
	{
		auto StringValue = FJavaHelper::ToJavaString(Env, ProductIDs[i]);
		Env->SetObjectArrayElement(*ProductIDArray, i, *StringValue);
	}

	Env->CallStaticVoidMethod(Class, Method, *ProductIDArray);
	
	Env->DeleteGlobalRef(Class);
#endif
}

void UAndroidBillingHelper::Purchase(FString ProductID)
{
#if PLATFORM_ANDROID
	LOG(LogMobileStorePurchaseSystem, "UE Billing Purchase Start")
	
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	if (!Env) return;

	jclass Class = FAndroidApplication::FindJavaClassGlobalRef("com/billing/unreal/UnrealBillingAndroid");
	if(!Class) return;

	auto Method = FJavaWrapper::FindStaticMethod(Env, Class, "purchase", "(Ljava/lang/String;)V", false);
	if(!Method) return;

	jstring PurchaseIDParam = Env->NewStringUTF(TCHAR_TO_UTF8(*ProductID));

	Env->CallStaticVoidMethod(Class, Method, PurchaseIDParam);

	Env->DeleteLocalRef(PurchaseIDParam);
	Env->DeleteGlobalRef(Class);
#endif
}

void UAndroidBillingHelper::FinalizePurchase(FAndroidPurchaseInfo PurchaseInfo, bool Consume)
{
#if PLATFORM_ANDROID
	LOG(LogMobileStorePurchaseSystem, "UE Billing Purchase Start")
	
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	if (!Env) return;

	jclass Class = FAndroidApplication::FindJavaClassGlobalRef("com/billing/unreal/UnrealBillingAndroid");
	if(!Class) return;

	auto Method = FJavaWrapper::FindStaticMethod(Env, Class, "finalizePurchase", "(Ljava/lang/String;Z)V", false);
	if(!Method) return;

	jstring ReceiptParam = Env->NewStringUTF(TCHAR_TO_UTF8(*PurchaseInfo.Token));

	Env->CallStaticVoidMethod(Class, Method, ReceiptParam, Consume);

	Env->DeleteLocalRef(ReceiptParam);
	Env->DeleteGlobalRef(Class);
#endif
}

#if PLATFORM_ANDROID

JNI_METHOD void Java_com_billing_unreal_UnrealBillingAndroid_onProductsPurchaseSuccessful(JNIEnv *env, jobject obj, jstring purchaseJSON, jstring signature)
{
	LOG_STATIC(LogMobileStorePurchaseSystem, "UE Billing Product Purchased")
	
	const FString JSONString = FJavaHelper::FStringFromParam(env, purchaseJSON);

	LOG_STATIC(LogMobileStorePurchaseSystem, "Product JSON: %s", *JSONString)

	// Purchase JSON example:
	//{ 
	//	"orderId":"GPA.3386-2124-6888-54771",
	//	"packageName":"marble.strike.battle.royale",
	//	"productId":"currency_crystals_1",
	//	"purchaseTime":1665439535789,
	//	"purchaseState":0,
	//	"purchaseToken":"okcconnmngpeffhfokpmpbie.AO-J1OysGrijDGzV6sipVFO-3Nf5WyXkpaFa7XPpMcVEOCzSXFJd6AamXcVDlGhPHsWGcGBD7xUrHcGGWAzrdQsVsz6kJwX_gH1FpAuNA1aY7mquS6m7pXs",
	//	"quantity":1,
	//	"acknowledged":false
	//}

	TSharedPtr<FJsonObject> PurchaseJson = MakeShareable(new FJsonObject);
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (FJsonSerializer::Deserialize(Reader, PurchaseJson))
	{
		FAndroidPurchaseInfo PurchaseInfo;
		PurchaseInfo.ProductID = PurchaseJson->GetStringField("productId");
		PurchaseInfo.Token = PurchaseJson->GetStringField("purchaseToken");
		PurchaseInfo.OrderID = PurchaseJson->GetStringField("orderId");
		PurchaseInfo.Signature = FJavaHelper::FStringFromParam(env, signature);
		PurchaseInfo.Details.Add("PurchaseTime", FString::FromInt(PurchaseJson->GetIntegerField("purchaseTime")));
		PurchaseInfo.Details.Add("Quantity",FString::FromInt(PurchaseJson->GetIntegerField("quantity")));
		PurchaseInfo.Details.Add("PurchaseState",FString::FromInt(PurchaseJson->GetIntegerField("purchaseState")));
		PurchaseInfo.Details.Add("Acknowledged", PurchaseJson->GetBoolField("acknowledged") ? "True" : "False");
	
		AsyncTask(ENamedThreads::GameThread, [PurchaseInfo]()
		{
			UAndroidBillingHelper::Get()->OnPurchaseSuccess.Broadcast(PurchaseInfo);
		});	
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, []()
		{
			UAndroidBillingHelper::Get()->OnPurchaseFail.Broadcast("", "Purchase JSON deserialization fail");
		});	
	}
};

JNI_METHOD void Java_com_billing_unreal_UnrealBillingAndroid_onProductsPurchaseError(JNIEnv *env, jobject obj, jstring Error)
{
	const FString ErrorString = FJavaHelper::FStringFromParam(env, Error);
	
	AsyncTask(ENamedThreads::GameThread, [ErrorString]()
	{
		UAndroidBillingHelper::Get()->OnPurchaseFail.Broadcast("", ErrorString);
	});	
};

JNI_METHOD void Java_com_billing_unreal_UnrealBillingAndroid_onProductsQuery(JNIEnv *env, jobject obj, jobjectArray productsDataJSON)
{
	if(!env) return;
		
	int ProductsNum = env->GetArrayLength(productsDataJSON);

	LOG_STATIC(LogMobileStorePurchaseSystem, "Recieved Products")
		
	if(ProductsNum <= 0) return;

	LOG_STATIC(LogMobileStorePurchaseSystem, "Products num: %i", ProductsNum)

	TArray<FAndroidProductInfo> ProductsInfo; 
		
	for (int i = 0; i < ProductsNum; ++i) {
			
		jstring objKey = (jstring) env->GetObjectArrayElement(productsDataJSON, i);

		const FString JSONString = FJavaHelper::FStringFromParam(env, objKey);

		LOG_STATIC(LogMobileStorePurchaseSystem, "Product JSON: %s", *JSONString)

		TSharedPtr<FJsonObject> MyJson = MakeShareable(new FJsonObject);
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

		if (FJsonSerializer::Deserialize(Reader, MyJson))
		{
			FAndroidProductInfo ProductInfo;

			ProductInfo.ProductID = MyJson->GetStringField("ProducID");
			ProductInfo.Name = MyJson->GetStringField("Name");
			ProductInfo.Description = MyJson->GetStringField("Description");
			ProductInfo.Type = MyJson->GetStringField("ProducType");
			ProductInfo.CurrencyCode = MyJson->GetStringField("CurrencyCode");
			ProductInfo.FormattedPrice = MyJson->GetStringField("FormattedPrice");
			ProductInfo.MicrosPrice = MyJson->GetIntegerField("Price");

			LOG_STATIC(LogMobileStorePurchaseSystem, "Product info deserialized")

			ProductsInfo.Add(ProductInfo);
		}

		LOG_STATIC(LogMobileStorePurchaseSystem, "Send Products To Unreal")
		
		AsyncTask(ENamedThreads::GameThread, [ProductsInfo]()
		{
			for(const FAndroidProductInfo& ProductInfo : ProductsInfo)
			{
				UAndroidBillingHelper::Get()->OnProductInfoReceive.Broadcast(ProductInfo);
			}
		});
	}
	
	
};

#endif