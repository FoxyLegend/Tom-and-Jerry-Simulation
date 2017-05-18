package com.example.admin.loginapp;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;

public class gameActivity extends AppCompatActivity implements View.OnClickListener {

    private FirebaseAuth mAuth;
    private TextView userEmail;
    private Button buttonLogout;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_game);

        mAuth = mAuth.getInstance();
        if(mAuth.getCurrentUser()==null)
        {
            finish();
            startActivity(new Intent(this, MainActivity.class));
        }
        FirebaseUser user = mAuth.getCurrentUser();

        userEmail = (TextView) findViewById(R.id.userEmail);
        userEmail.setText(user.getEmail());
        buttonLogout = (Button)findViewById(R.id.logout);
        buttonLogout.setOnClickListener(this);





    }

    @Override
    public void onClick(View v) {
        if (v == buttonLogout)
        {
            mAuth.signOut();
            finish();
            startActivity(new Intent(this, MainActivity.class));
        }
    }
}
