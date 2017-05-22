package com.example.admin.loginapp;

import android.*;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.support.annotation.Keep;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

public class GameActivity extends AppCompatActivity implements View.OnClickListener {

    private FirebaseAuth mAuth;
    private TextView userEmail;
    private Button buttonJoin, buttonLogout;

    final private DatabaseReference
            mDatabase = FirebaseDatabase.getInstance().getReference("gameroom");
    private final static String TAG = "GameActivity";

    FirebaseUser user;
    boolean enableJoinButton = false;
    boolean buttonAvailable = false;
    String myUID = null;

    // GPS
    LocationManager locationManager;
    LocationListener locationListener;
    boolean isGPSEnabled, isNetworkEnabled;
    private static final int MY_PERMISSIONS_REQUEST_GPS = 1;
    double lat, lng;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_game);


        mAuth = mAuth.getInstance();
        if(mAuth.getCurrentUser()==null)
        {
            finish();
            startActivity(new Intent(this, MainActivity.class));
        }user = mAuth.getCurrentUser();

        userEmail = (TextView) findViewById(R.id.userEmail);
        userEmail.setText("NOT INITIALIZED");

        buttonLogout = (Button)findViewById(R.id.logout_button);
        buttonLogout.setOnClickListener(this);
        buttonJoin = (Button)findViewById(R.id.join_button);
        buttonJoin.setOnClickListener(this);

        // Attach a listener to read the data at our posts reference
        mDatabase.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {
                if(dataSnapshot.exists() && dataSnapshot.child("creator").exists() && dataSnapshot.child("state").exists()){
                    String txt = "";
                    Member member;
                        buttonAvailable = false;
                        enableJoinButton = true;
                        for (DataSnapshot ds : dataSnapshot.child("/members").getChildren()){
                            myUID = ds.getKey().toString();
                        member = ds.getValue(Member.class);
                        txt += "members: " + member.email + "(" + member.lat + ", " + member.lng + ")\n";

                        if(user.getEmail().equalsIgnoreCase(member.email)){
                            enableJoinButton = false;
                        }
                    }

                    if(enableJoinButton){
                        buttonJoin.setText("Join");
                    }
                    else {
                        buttonJoin.setText("Exit");
                    }

                    userEmail.setText(
                            dataSnapshot.child("creator").getValue().toString() + "\n"
                            + txt
                            + dataSnapshot.child("members").getChildrenCount() + "\n"
                            + dataSnapshot.child("state").getValue().toString()
                    );
                    Log.e(TAG, "Data loaded successfully." + dataSnapshot.getValue());


                    if(dataSnapshot.child("state").getValue().toString().equalsIgnoreCase("ready"))
                        buttonAvailable = true;
                    else
                        buttonAvailable = false;
                }
                else {
                    userEmail.setText("*** There is no game room ***");
                    Log.e(TAG, "Data not exist.");
                }
            }

            @Override
            public void onCancelled(DatabaseError databaseError) {
                System.out.println("The read failed: " + databaseError.getCode());
            }

        });

        locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
        locationListener = new LocationListener() {
            public void onLocationChanged(Location location) {
                lat = location.getLatitude();
                lng = location.getLongitude();

                if(buttonAvailable){
                    if(!enableJoinButton){ // already joined state
                        mDatabase.child("members").child(myUID).child("lat").setValue(lat);
                        mDatabase.child("members").child(myUID).child("lng").setValue(lng);

                    }
                }
                Log.w(TAG, "latitude: " + lat + ", longitude: " + lng);
            }

            public void onStatusChanged(String provider, int status, Bundle extras) {
                Log.w(TAG, "onStatusChanged");
            }

            public void onProviderEnabled(String provider) {
                Log.w(TAG, "onProviderEnabled");
            }

            public void onProviderDisabled(String provider) {
                Log.w(TAG, "onProviderDisabled");
            }
        };
        // GPS 프로바이더 사용가능여부
        isGPSEnabled = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
        // 네트워크 프로바이더 사용가능여부
        isNetworkEnabled = locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER);

        registerLocationUpdates();
    }

    private void registerLocationUpdates() {
        // Register the listener with the Location Manager to receive location updates
        if (ActivityCompat.checkSelfPermission(getBaseContext(), android.Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED
                && ActivityCompat.checkSelfPermission(getBaseContext(), android.Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            // 권한 획득에 대한 설명 보여주기
            if (ActivityCompat.shouldShowRequestPermissionRationale(getParent(),
                    android.Manifest.permission.ACCESS_FINE_LOCATION)) {

                // 사용자에게 권한 획득에 대한 설명을 보여준 후 권한 요청을 수행

            } else {

                // 권한 획득의 필요성을 설명할 필요가 없을 때는 아래 코드를
                //수행해서 권한 획득 여부를 요청한다.

                ActivityCompat.requestPermissions(this,
                        new String[]{ android.Manifest.permission.ACCESS_FINE_LOCATION, android.Manifest.permission.ACCESS_COARSE_LOCATION},
                        MY_PERMISSIONS_REQUEST_GPS);

            }
        }
        locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER,
                1000, 1, locationListener);
        locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER,
                1000, 1, locationListener);
        //1000은 1초마다, 1은 1미터마다 해당 값을 갱신한다는 뜻으로, 딜레이마다 호출하기도 하지만
        //위치값을 판별하여 일정 미터단위 움직임이 발생 했을 때에도 리스너를 호출 할 수 있다.
    }

    void synchronizeGPSLocation(){
        // Register the listener with the Location Manager to receive location updates
        if (ActivityCompat.checkSelfPermission(getBaseContext(), android.Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED
                && ActivityCompat.checkSelfPermission(getBaseContext(), android.Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            // 권한 획득에 대한 설명 보여주기
            if (ActivityCompat.shouldShowRequestPermissionRationale(getParent(),
                    android.Manifest.permission.ACCESS_FINE_LOCATION)) {

                // 사용자에게 권한 획득에 대한 설명을 보여준 후 권한 요청을 수행

            } else {

                // 권한 획득의 필요성을 설명할 필요가 없을 때는 아래 코드를
                //수행해서 권한 획득 여부를 요청한다.

                ActivityCompat.requestPermissions(this,
                        new String[]{ android.Manifest.permission.ACCESS_FINE_LOCATION, android.Manifest.permission.ACCESS_COARSE_LOCATION},
                        MY_PERMISSIONS_REQUEST_GPS);

            }
        }
        locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, locationListener);
        locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, locationListener);

        String locationProvider = LocationManager.GPS_PROVIDER;
        Location lastKnownLocation = locationManager.getLastKnownLocation(locationProvider);
        if (lastKnownLocation != null) {
            lng = lastKnownLocation.getLongitude();
            lat = lastKnownLocation.getLatitude();
            Log.d(TAG, "longitude=" + lng + ", latitude=" + lat);
        }
        else {
        }
    }

    public static final class Member{
        public String email;
        public double lat;
        public double lng;

        public Member(){}
        public Member(String email, double lat, double lng){
            this.email = email;
            this.lat = lat;
            this.lng = lng;
        }
    }

    @Override
    public void onClick(View v) {
        if (v == buttonLogout)
        {
            mAuth.signOut();
            finish();
            startActivity(new Intent(this, MainActivity.class));
        }
        if (v == buttonJoin)
        {
            if (buttonAvailable){
                if(enableJoinButton){
                    //synchronizeGPSLocation();
                    mDatabase.child("members").push().setValue(new Member(user.getEmail(), lat, lng));
                }
                else {

                    mDatabase.child("members").child(myUID).removeValue();
                }
            }
        }
    }
}
