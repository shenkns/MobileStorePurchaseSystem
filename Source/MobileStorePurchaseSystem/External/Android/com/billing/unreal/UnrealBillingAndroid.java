package com.billing.unreal;

import android.app.NativeActivity;
import java.lang.Thread;
import android.util.Log;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.stream.Collectors;
import java.util.Collections;
import java.util.concurrent.ConcurrentHashMap;
import com.google.common.collect.ImmutableList;
import com.android.vending.billing.util.Base64;
import org.json.JSONObject;
import org.json.JSONException;

import com.android.billingclient.api.*;
import com.android.billingclient.api.BillingClient.*;
import com.android.billingclient.api.BillingFlowParams.ProductDetailsParams;
import com.android.billingclient.api.ProductDetails.OneTimePurchaseOfferDetails;
import com.android.billingclient.api.QueryProductDetailsParams.Product;

import java.lang.reflect.*;

public class UnrealBillingAndroid
{
    private static UnrealBillingAndroid unrealBilling;
    
    private NativeActivity activity;
    private PurchasesUpdatedListener purchasesListener; 
    private BillingClient billingClient;
    
    private ConcurrentHashMap<String, ProductDetails> purchaseDetails;
    
    // Callbacks
    private native static void onProductsQuery(String[] ProductsJSON);
    private native static void onProductsPurchaseSuccessful(String PurchaseJSON, String Signature);
    private native static void onProductsPurchaseError(String Error);
    
    static public void queryProducts(String[] ProductsIDs){
        if(unrealBilling == null) {
            Log.e("Billing", "No unreal billing initialized!");
            return;
        }
        unrealBilling.queryProducts_Internal(ProductsIDs);
    }
    
    static public void purchase(String ProductID){
        if(unrealBilling == null) {
            Log.e("Billing", "No unreal billing initialized!");
            return;
        }
        unrealBilling.purchase_Internal(ProductID);
    }
    
    static public void finalizePurchase(String PurchaseToken, boolean Consume){
        if(unrealBilling == null) {
            Log.e("Billing", "No unreal billing initialized!");
            return;
        }
        unrealBilling.finalizePurchase_Internal(PurchaseToken, Consume);
    }
   
    public void init(NativeActivity appActivity)
    {
        unrealBilling = this;
        
        activity = appActivity;
        
        purchaseDetails = new ConcurrentHashMap();
        
        Log.d("Billing", "Billing init!");
        
        purchasesListener = new PurchasesUpdatedListener() {
            @Override
            public void onPurchasesUpdated(BillingResult billingResult, List<Purchase> purchases) {
                if (billingResult.getResponseCode() == BillingResponseCode.OK && purchases.size() > 0) {
                    
                    for(Purchase purchase : purchases)
                    {
                        String receipt = purchase.getOriginalJson();
                        
                        Log.d("Billing", "Purchase successful: " + receipt);

                        onProductsPurchaseSuccessful(receipt, purchase.getSignature());
                    }
                }
                else
                {
                    Log.d("Billing", "Purchase error: " + billingResult.getDebugMessage());
                    
                    onProductsPurchaseError(billingResult.getDebugMessage());
                }
            }
        };
        
        billingClient = BillingClient.newBuilder(activity)
            .setListener(purchasesListener)
            .enablePendingPurchases()
            .build();
        
        connectToBilling();
    }
    
    private void connectToBilling()
    {
        Log.d("Billing", "Billing Connecting..");
        
        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(BillingResult billingResult) {
                if (billingResult.getResponseCode() ==  BillingResponseCode.OK) {
                   Log.d("Billing", "Billing connected!");
                }
            }
            @Override
            public void onBillingServiceDisconnected() {
                // Try to restart the connection on the next request to
                // Google Play by calling the startConnection() method.
                Log.d("Billing", "Billing disconnected!");
            }
        });
    }
    
    private void queryProducts_Internal(String[] ProductsIDs)
    {
        Log.d("Billing", "Query products...");
        Log.d("Billing", "Query products thread:" + Thread.currentThread().getName());
        
        List<String> products = Arrays.asList(ProductsIDs);
        
        List<Product> productList = new ArrayList<Product>();
        
        for(String Id : products){
            Log.d("Billing", "Query product: " + Id);
            
            productList.add(
                Product.newBuilder()
               .setProductId(Id)
               .setProductType(BillingClient.ProductType.INAPP)
               .build()
            );
        }
        
        QueryProductDetailsParams queryProductDetailsParams = QueryProductDetailsParams.newBuilder()
                .setProductList(productList)
                .build();
        
        billingClient.queryProductDetailsAsync(
            queryProductDetailsParams,
            new ProductDetailsResponseListener() {
                public void onProductDetailsResponse(BillingResult billingResult, List<ProductDetails> productDetailsList) {
                    if (billingResult.getResponseCode() ==  BillingResponseCode.OK) {   
                        int detailsAmount = productDetailsList.size();
                        Log.d("Billing", "Query success. Amount of products:" + detailsAmount);
                        Log.d("Billing", "Query success. Thread:" + Thread.currentThread().getName());
                        
                        String[] ProductsJSON = new String[detailsAmount];
                        
                        for(int i=0; i < detailsAmount; i++){
                            ProductDetails details = productDetailsList.get(i);
                            
                            Log.d("Billing", "Product info: " + details.toString());
                            
                            try {
                                JSONObject productJSON = new JSONObject();
                                productJSON.put("ProducID", details.getProductId());
                                productJSON.put("ProducType", details.getProductType());
                                productJSON.put("Name", details.getName());
                                productJSON.put("Description", details.getDescription());
                                
                                OneTimePurchaseOfferDetails OneTimePurchaseDetails = details.getOneTimePurchaseOfferDetails();
                                
                                if(OneTimePurchaseDetails != null){
                                    productJSON.put("Price", OneTimePurchaseDetails.getPriceAmountMicros());
                                    productJSON.put("FormattedPrice", OneTimePurchaseDetails.getFormattedPrice());
                                    productJSON.put("CurrencyCode", OneTimePurchaseDetails.getPriceCurrencyCode());
                                }
                                
                                ProductsJSON[i] = productJSON.toString();
                                
                                purchaseDetails.put(details.getProductId(), details);
                            } catch (JSONException e) {
                                Log.e("Billing", "Failed to create JSON for product!");
                            }
                        }
                        
                        Log.d("Billing", "Send products to unreal");
                        onProductsQuery(ProductsJSON); // Send to Unreal
                    }
                    else {
                        Log.d("Billing", "Query error: " + billingResult.getDebugMessage());
                    }
                }
            }
        );
    }

    private void purchase_Internal(String ProductID)
    {
        Log.d("Billing", "Start purchase...");
        Log.d("Billing", "Product Details: "+purchaseDetails.size());
        
        ProductDetails Details = purchaseDetails.get(ProductID);
        
        if(Details == null){
            Log.w("Billing", "No details for purchase: " + ProductID);
        }
        
        ImmutableList productDetailsParamsList =
            ImmutableList.of(
                ProductDetailsParams.newBuilder()
                    .setProductDetails(Details)
                    .build()
            );
        
        BillingFlowParams billingFlowParams = BillingFlowParams.newBuilder()
            .setProductDetailsParamsList(productDetailsParamsList)
            .setIsOfferPersonalized(false)
            .build();
        
        BillingResult billingResult = billingClient.launchBillingFlow(activity, billingFlowParams);
    }
    
    private void finalizePurchase_Internal(String PurchaseToken, boolean Consume)
    {
        Log.d("Billing", "Finalizing purchase...");
        
        if(Consume){
            ConsumeParams consumeParams =
                ConsumeParams.newBuilder()
                .setPurchaseToken(PurchaseToken)
                .build();
        
            ConsumeResponseListener listener = new ConsumeResponseListener() {
                @Override
                public void onConsumeResponse(BillingResult billingResult, String purchaseToken) {
                    if (billingResult.getResponseCode() == BillingResponseCode.OK) {
                        // TODO: Handle consume operation.
                    }
                }
            };
        
            billingClient.consumeAsync(consumeParams, listener);
        }
        else 
        {
            AcknowledgePurchaseResponseListener acknowledgePurchaseResponseListener = new AcknowledgePurchaseResponseListener() {
                @Override
                public void onAcknowledgePurchaseResponse(BillingResult billingResult){
                    // TODO: Handle acknowledge
                }
            };
            
            AcknowledgePurchaseParams acknowledgePurchaseParams =
                AcknowledgePurchaseParams.newBuilder()
                .setPurchaseToken(PurchaseToken)
                .build();
            
            billingClient.acknowledgePurchase(acknowledgePurchaseParams, acknowledgePurchaseResponseListener);
        }
    }
}
