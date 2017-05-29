package com.example.admin.loginapp;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.ImageView;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

public class MapActivity extends AppCompatActivity {

    final private DatabaseReference
            mDatabase = FirebaseDatabase.getInstance().getReference("gameroom");
    ImageView imgView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_map);

        imgView = (ImageView) findViewById(R.id.mapImageView);
        Bitmap myMap = BitmapFactory.decodeResource(getResources(), R.drawable.kaistmap);

        double ax = 127.3645189;
        double ay = 36.3741570;

        double mapx = 127.3623389;
        double mapy = 36.3706170;

        int px = (int)(((ax - mapx) * 1e7 + 80000) * myMap.getWidth() / 160000);
        int py = (int)(((ay - mapy) * 1e7 + 80000) * myMap.getHeight() / 160000);

        Bitmap backBit = Bitmap.createBitmap(myMap.getWidth()*2, myMap.getHeight()*2, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(backBit);
        canvas.drawARGB(255, 225, 225, 255);
        canvas.drawBitmap(myMap, myMap.getWidth()/2, myMap.getHeight()/2, null);

        int width = backBit.getWidth();
        int height = backBit.getHeight();

        int margin = myMap.getWidth() / 2;
        int cropwidth = width / 20;
        int cropheight = cropwidth * 3 / 4;
        backBit = Bitmap.createBitmap(backBit, px - cropwidth + margin, backBit.getHeight() - (py + margin) - cropheight, cropwidth*2, cropheight*2);
        //backBit = Bitmap.createScaledBitmap(backBit, width, width * 3 / 4, true);

        imgView.setImageBitmap(backBit);

        mDatabase.child("state").addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {
                if(!dataSnapshot.exists() || !dataSnapshot.getValue().toString().equalsIgnoreCase("started")){
                    mDatabase.child("state").removeEventListener(this);
                    startActivity(new Intent(getBaseContext(), GameActivity.class));
                }
            }

            @Override
            public void onCancelled(DatabaseError databaseError) {

            }
        });
    }
}
