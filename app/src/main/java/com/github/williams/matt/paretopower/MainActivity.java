package com.github.williams.matt.paretopower;

import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import java.text.DateFormat;
import java.text.NumberFormat;
import java.util.Date;
import java.util.TimeZone;
import java.util.UUID;

import go.ParetoPower.*;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private static final String PARETO_POWER = "ParetoPower";

    private Handler mHandler = new Handler();
    private UUID mUuid = UUID.randomUUID();
    private JSONObject mHceCard;

    int mBatteryLevel;
    int mChargeRate = -1;
    int mChargeDuration;
    private ProgressBar mProgressBar;
    private TextView mBatteryPrompt;
    private TextView mChargePrompt;
    private TextView mNotesPrompt;

    void setBatteryLevel(int level) {
        mBatteryLevel = level;
        mProgressBar.setProgress(mBatteryLevel);
        mBatteryPrompt.setText("Battery Status: " + level + "%");
    }

    Runnable mBatteryUpdate = new Runnable() {
        @Override
        public void run() {
            synchronized (MainActivity.this) {
                setBatteryLevel(Math.min(Math.max(mBatteryLevel + mChargeRate, 0), 100));
                mChargeDuration = Math.max(mChargeDuration - 1, 0);
                if (mChargeDuration == 0) {
                    mChargeRate = -1;
                    mChargePrompt.setText("Not charging");
                }
            }
            mHandler.postDelayed(this, 500);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        mProgressBar = (ProgressBar)findViewById(R.id.progressBar);
        mBatteryPrompt = (TextView)findViewById(R.id.textView);
        mChargePrompt = (TextView)findViewById(R.id.textView2);
        mNotesPrompt = (TextView)findViewById(R.id.textView3);
        setBatteryLevel(50);
        mHandler.postDelayed(mBatteryUpdate, 500);

        try {
            mHceCard = new JSONObject();
            mHceCard.put("FirstName", "Matt");
            mHceCard.put("LastName", "Williams");
            mHceCard.put("ExpMonth", 10);
            mHceCard.put("ExpYear", 2018);
            mHceCard.put("CardNumber", "4444333322221111");
            mHceCard.put("Type", "Card");
            mHceCard.put("Cvc", "123");
        } catch (JSONException e) {
            e.printStackTrace();
        }

        new Thread() {
            @Override
            public void run() {
                try {
                    Wrapper wrapper = ParetoPower.initialise("ParetoPower", "Pareto-efficient Sustainable Power");
                    while (true) {
                        try {
                            String devicesStr = wrapper.deviceDiscovery(2000);
                            Log.e(TAG, "DeviceDiscovery: " + devicesStr);
                            if ((devicesStr != null) && (!devicesStr.equals("null"))) {
                                JSONArray devices = new JSONArray(devicesStr);
                                for (int ii = 0; ii < devices.length(); ii++) {
                                    JSONObject device = devices.getJSONObject(0);
                                    String deviceDescription = device.getString("deviceDescription");

                                    if (deviceDescription.equals(PARETO_POWER)) {
                                        wrapper.initConsumer(device.getString("scheme"), device.getString("hostname"), device.getInt("portNumber"), device.getString("urlPrefix"), mUuid.toString(), mHceCard.toString());
                                        String servicesStr = wrapper.requestServices();
                                        Log.e(TAG, "RequestServices: " + servicesStr);
                                        if (servicesStr != null) {
                                            JSONArray services = new JSONArray(servicesStr);

                                            if (services.length() > 0) {
                                                JSONObject service = services.getJSONObject(0);
                                                while (true) {
                                                    String pricesStr = wrapper.getServicePrices(service.getInt("ServiceID"));
                                                    Log.e(TAG, "GetServicePrices: " + pricesStr);
                                                    if (pricesStr != null) {
                                                        JSONArray prices = new JSONArray(pricesStr);
                                                        if (prices.length() > 0) {
                                                            JSONObject bestPrice = prices.getJSONObject(0);

                                                            String notes = "Available prices (as of " + DateFormat.getTimeInstance().format(new Date()) + "):\n";
                                                            for (int jj = 0; jj < prices.length(); jj++) {
                                                                JSONObject price = prices.getJSONObject(jj);
                                                                JSONObject pricePerUnit = price.getJSONObject("pricePerUnit");

                                                                notes += String.format(price.getString("priceDescription") + " - " +
                                                                                       NumberFormat.getCurrencyInstance().format(pricePerUnit.getInt("amount") / 100.0) +
                                                                                       " " + pricePerUnit.getString("currencyCode") +
                                                                                       " / " + price.getString("unitDescription") + "\n");

                                                                if (pricePerUnit.getInt("amount") <= bestPrice.getJSONObject("pricePerUnit").getInt("amount")) {
                                                                    bestPrice = price;
                                                                }
                                                            }
                                                            final String notes2 = notes;
                                                            mHandler.post(new Runnable() {
                                                                @Override
                                                                public void run() {
                                                                    mNotesPrompt.setText(notes2);
                                                                }
                                                            });

                                                            int batteryLevel;
                                                            int chargeRate;
                                                            synchronized (MainActivity.this) {
                                                                batteryLevel = mBatteryLevel;
                                                                chargeRate = mChargeRate;
                                                            }

                                                            if (chargeRate < 0) {
                                                                String chargeString = "";
                                                                if (batteryLevel < 30) {
                                                                    chargeString = "Charging using \"" + bestPrice.getString("priceDescription") + "\" - battery critical!";
                                                                } else if ((batteryLevel < 60) && (bestPrice.getJSONObject("pricePerUnit").getInt("amount") < 10)) {
                                                                    chargeString = "Opportunistically charging using \"" + bestPrice.getString("priceDescription") + "\"";
                                                                }
                                                                if (!chargeString.equals("")) {
                                                                    String totalStr = wrapper.selectService(service.getInt("ServiceID"), 10, bestPrice.getInt("priceID"));
                                                                    Log.e(TAG, "SelectService: " + totalStr);
                                                                    if ((totalStr != null) && (!totalStr.equals("null"))) {
                                                                        JSONObject total = new JSONObject(totalStr);
                                                                        String paymentStr = wrapper.makePayment(totalStr);
                                                                        Log.e(TAG, "MakePayment: " + paymentStr);
                                                                        if ((paymentStr != null) && (!paymentStr.equals("null"))) {
                                                                            JSONObject payment = new JSONObject(paymentStr);
                                                                            JSONObject token = payment.getJSONObject("serviceDeliveryToken");
                                                                            String beginResponse = wrapper.beginServiceDelivery(service.getInt("ServiceID"), token.toString(), 10);
                                                                            Log.e(TAG, "BeginServiceDelivery: " + beginResponse);
                                                                            synchronized (MainActivity.this) {
                                                                                mChargeRate = 2;
                                                                                mChargeDuration = 20;
                                                                            }
                                                                            final String chargeString2 = chargeString;
                                                                            mHandler.post(new Runnable() {
                                                                                @Override
                                                                                public void run() {
                                                                                    mChargePrompt.setText(chargeString2);
                                                                                }
                                                                            });
                                                                            //String endResponse = wrapper.endServiceDelivery(service.getInt("ServiceID"), token.toString(), 10);
                                                                            //Log.e(TAG, "EndServiceDelivery: " + endResponse);
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                    Thread.sleep(2000, 0);
                                                }
/*
                                                mHandler.post(new Runnable() {
                                                    @Override
                                                    public void run() {
                                                        Toast toast = Toast.makeText(getApplicationContext(), deviceDescription, Toast.LENGTH_SHORT);
                                                        toast.show();
                                                    }
                                                });
*/
                                            }
                                        }
                                    }
                                }
                            }
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }.start();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
