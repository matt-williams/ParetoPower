package com.github.williams.matt.paretopower;

import android.os.Bundle;
import android.os.Handler;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import java.util.UUID;

import go.ParetoPower.*;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private static final String PARETO_POWER = "ParetoPower";

    private Handler mHandler = new Handler();
    private UUID mUuid = UUID.randomUUID();
    private JSONObject hceCard;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        try {
            hceCard = new JSONObject();
            hceCard.put("FirstName", "Matt");
            hceCard.put("LastName", "Williams");
            hceCard.put("ExpMonth", 10);
            hceCard.put("ExpYear", 2018);
            hceCard.put("CardNumber", "4444333322221111");
            hceCard.put("Type", "Card");
            hceCard.put("Cvc", "123");
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
                            String devicesStr = wrapper.deviceDiscovery(5000);
                            Log.e(TAG, "DeviceDiscovery: " + devicesStr);
                            if (devicesStr != null) {
                                JSONArray devices = new JSONArray(devicesStr);
                                for (int ii = 0; ii < devices.length(); ii++) {
                                    JSONObject device = devices.getJSONObject(0);
                                    String deviceDescription = device.getString("deviceDescription");

                                    if (deviceDescription.equals(PARETO_POWER)) {
                                        wrapper.initConsumer(device.getString("scheme"), device.getString("hostname"), device.getInt("portNumber"), device.getString("urlPrefix"), mUuid.toString(), hceCard.toString());
                                        String servicesStr = wrapper.requestServices();
                                        Log.e(TAG, "RequestServices: " + servicesStr);
                                        if (servicesStr != null) {
                                            JSONArray services = new JSONArray(servicesStr);

                                            if (services.length() > 0) {
                                                JSONObject service = services.getJSONObject(0);
                                                String pricesStr = wrapper.getServicePrices(service.getInt("ServiceID"));
                                                Log.e(TAG, "GetServicePrices: " + pricesStr);
                                                if (pricesStr != null) {
                                                    JSONArray prices = new JSONArray(pricesStr);
                                                    if (prices.length() > 0) {
                                                        JSONObject price = prices.getJSONObject(0);
                                                        String totalStr = wrapper.selectService(service.getInt("ServiceID"), 10, price.getInt("priceID"));
                                                        Log.e(TAG, "SelectService: " + totalStr);
                                                        if (totalStr != null) {
                                                            JSONObject total = new JSONObject(totalStr);
                                                            String paymentStr = wrapper.makePayment(totalStr);
                                                            Log.e(TAG, "MakePayment: " + paymentStr);
                                                            if (paymentStr != null) {
                                                                JSONObject payment = new JSONObject(paymentStr);
                                                                JSONObject token = payment.getJSONObject("serviceDeliveryToken");
                                                                String beginResponse = wrapper.beginServiceDelivery(service.getInt("ServiceID"), token.toString(), 10);
                                                                Log.e(TAG, "BeginServiceDelivery: " + beginResponse);
                                                                String endResponse = wrapper.endServiceDelivery(service.getInt("ServiceID"), token.toString(), 10);
                                                                Log.e(TAG, "EndServiceDelivery: " + endResponse);
                                                            }
                                                        }
                                                    }
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
