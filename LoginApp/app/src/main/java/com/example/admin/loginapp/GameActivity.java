package com.example.admin.loginapp;

import android.*;
import android.Manifest;
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
import android.widget.BaseExpandableListAdapter;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.TextView;
import android.widget.Toast;

import com.google.firebase.appindexing.Action;
import com.google.firebase.appindexing.FirebaseUserActions;
import com.google.firebase.appindexing.builders.Actions;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.util.ArrayList;

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

    // elistview
    private ArrayList<String> mGroupList = null;
    private ArrayList<ArrayList<String>> mChildList = null;
    private ArrayList<String> mFoxListContent = null;
    private ArrayList<String> mHoundListContent = null;
    private ExpandableListView mListView;
    private BaseExpandableListAdapter mBaseAdapter;

    private boolean foxExist = true;
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

        Log.e(TAG, "Authorized anyway");

        userEmail = (TextView) findViewById(R.id.textView);
        userEmail.setText("NOT INITIALIZED");

        buttonLogout = (Button)findViewById(R.id.logout_button);
        buttonLogout.setOnClickListener(this);
        buttonJoin = (Button)findViewById(R.id.join_button);
        buttonJoin.setOnClickListener(this);


        mListView = (ExpandableListView) findViewById(R.id.elv_list);
        mGroupList = new ArrayList<String>();
        mChildList = new ArrayList<ArrayList<String>>();
        mFoxListContent = new ArrayList<String>();
        mHoundListContent = new ArrayList<String>();

        mGroupList.add("FOXES");
        mGroupList.add("HOUNDS");


        mChildList.add(mFoxListContent);
        mChildList.add(mHoundListContent);

        mBaseAdapter = new BaseExpandableAdapter(this, mGroupList, mChildList);
        mListView.setAdapter(mBaseAdapter);

        // 그룹 클릭 했을 경우 이벤트
        mListView.setOnGroupClickListener(new ExpandableListView.OnGroupClickListener() {
            @Override
            public boolean onGroupClick(ExpandableListView parent, View v,
                                        int groupPosition, long id) {
                //Toast.makeText(getApplicationContext(), "g click = " + groupPosition,
                        //Toast.LENGTH_SHORT).show();
                if(buttonAvailable){
                    if(!enableJoinButton){ // already joined state
                        if(groupPosition == 0){
                            mDatabase.child("members").child(myUID).child("role").setValue("fox");
                        }
                        else if(groupPosition == 1){
                            mDatabase.child("members").child(myUID).child("role").setValue("hound");
                        }

                    }
                }
                return true;
            }
        });

        // 차일드 클릭 했을 경우 이벤트
        mListView.setOnChildClickListener(new ExpandableListView.OnChildClickListener() {
            @Override
            public boolean onChildClick(ExpandableListView parent, View v,
                                        int groupPosition, int childPosition, long id) {
                //Toast.makeText(getApplicationContext(), "c click = " + childPosition,
                        //Toast.LENGTH_SHORT).show();
                return true;
            }
        });

        // 그룹이 닫힐 경우 이벤트
        mListView.setOnGroupCollapseListener(new ExpandableListView.OnGroupCollapseListener() {
            @Override
            public void onGroupCollapse(int groupPosition) {
                //Toast.makeText(getApplicationContext(), "g Collapse = " + groupPosition,
                        //Toast.LENGTH_SHORT).show();
            }
        });

        // 그룹이 열릴 경우 이벤트
        mListView.setOnGroupExpandListener(new ExpandableListView.OnGroupExpandListener() {
            @Override
            public void onGroupExpand(int groupPosition) {
                //Toast.makeText(getApplicationContext(), "g Expand = " + groupPosition,
                        //Toast.LENGTH_SHORT).show();

            }
        });


        // Attach a listener to read the data at our posts reference
        mDatabase.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {
                buttonAvailable = false;
                enableJoinButton = true;
                mFoxListContent.clear();
                mHoundListContent.clear();
                foxExist = false;
                if(dataSnapshot.exists() && dataSnapshot.child("creator").exists() && dataSnapshot.child("state").exists()){
                    if(dataSnapshot.child("state").getValue().toString().equalsIgnoreCase("started")){

                    }
                    else {
                        String txt = "";
                        Member member;
                        for (DataSnapshot ds : dataSnapshot.child("/members").getChildren()){
                            member = ds.getValue(Member.class);
                            txt += "members: " + member.email + "[" + member.role + "]" + "\n\t(" + member.lat + ", " + member.lng + ")\n";

                            if(user.getEmail().equalsIgnoreCase(member.email)){
                                enableJoinButton = false;
                                myUID = ds.getKey().toString();
                            }
                            if(member.role.equalsIgnoreCase("fox")){
                                mFoxListContent.add(member.email);
                                foxExist = true;
                            }
                            if(member.role.equalsIgnoreCase("hound")){
                                mHoundListContent.add(member.email);
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

                        mBaseAdapter.notifyDataSetChanged();
                    }
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
        if (ActivityCompat.checkSelfPermission(getBaseContext(), Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED
                && ActivityCompat.checkSelfPermission(getBaseContext(), Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            // 권한 획득에 대한 설명 보여주기
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.ACCESS_FINE_LOCATION)) {

                // 사용자에게 권한 획득에 대한 설명을 보여준 후 권한 요청을 수행

            } else {

                // 권한 획득의 필요성을 설명할 필요가 없을 때는 아래 코드를
                //수행해서 권한 획득 여부를 요청한다.

                ActivityCompat.requestPermissions(this,
                        new String[]{ Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION},
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
        if (ActivityCompat.checkSelfPermission(getBaseContext(), Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED
                && ActivityCompat.checkSelfPermission(getBaseContext(), Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            // 권한 획득에 대한 설명 보여주기
            if (ActivityCompat.shouldShowRequestPermissionRationale(getParent(),
                    Manifest.permission.ACCESS_FINE_LOCATION)) {

                // 사용자에게 권한 획득에 대한 설명을 보여준 후 권한 요청을 수행

            } else {

                // 권한 획득의 필요성을 설명할 필요가 없을 때는 아래 코드를
                //수행해서 권한 획득 여부를 요청한다.

                ActivityCompat.requestPermissions(this,
                        new String[]{ Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION},
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

    /**
     * ATTENTION: This was auto-generated to implement the App Indexing API.
     * See https://g.co/AppIndexing/AndroidStudio for more information.
     */
    public Action getIndexApiAction() {
        return Actions.newView("Game", "http://[ENTER-YOUR-URL-HERE]");
    }

    @Override
    public void onStart() {
        super.onStart();

        // ATTENTION: This was auto-generated to implement the App Indexing API.
        // See https://g.co/AppIndexing/AndroidStudio for more information.
        FirebaseUserActions.getInstance().start(getIndexApiAction());
    }

    @Override
    public void onStop() {

        // ATTENTION: This was auto-generated to implement the App Indexing API.
        // See https://g.co/AppIndexing/AndroidStudio for more information.
        FirebaseUserActions.getInstance().end(getIndexApiAction());
        super.onStop();
    }

    public static final class Member{
        public String email;
        public double lat;
        public double lng;
        public String role;

        public Member(){}
        public Member(String email, double lat, double lng, String role){
            this.email = email;
            this.lat = lat;
            this.lng = lng;
            this.role = role;
        }
    }

    @Override
    public void onClick(View v) {
        if (v == buttonLogout)
        {
            if(myUID != null)
                mDatabase.child("members").child(myUID).removeValue();
            mAuth.signOut();
            finish();
            startActivity(new Intent(this, MainActivity.class));
        }
        if (v == buttonJoin)
        {
            if (buttonAvailable){
                if(enableJoinButton){
                    //synchronizeGPSLocation();
                    mDatabase.child("members").push().setValue(new Member(user.getEmail(), lat, lng, "hound"));
                }
                else {
                    mDatabase.child("members").child(myUID).removeValue();
                }
            }
        }
    }
}
