package com.example.admin.loginapp;

import android.app.ProgressDialog;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.Toast;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.auth.AuthResult;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{

    private FirebaseAuth mAuth;

    private Button LoginButton;
    private Button RegisterButton;
    private EditText editTextemail;
    private EditText editTextpass;

    private ProgressDialog progressDialog;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mAuth = FirebaseAuth.getInstance();
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        progressDialog = new ProgressDialog(this);
        LoginButton = (Button) findViewById(R.id.LoginButton);
        RegisterButton = (Button) findViewById(R.id.RegisterButton);
        editTextemail = (EditText)findViewById(R.id.editTextemail);
        editTextpass = (EditText)findViewById(R.id.editTextpass);

        RegisterButton.setOnClickListener( this);
        LoginButton.setOnClickListener(this);


    }

    public void onClick(View view)
    {
        if(view==LoginButton)
        {
            //Login
            userLogin();
        }

        if(view==RegisterButton)
        {
            //Register
            registerUser();

        }

    }

    private void registerUser()
    {
        String email = editTextemail.getText().toString().trim();
        String password = editTextpass.getText().toString().trim();

        if(TextUtils.isEmpty(email))
        {
            //email empty
            Toast.makeText(this, "Please enter email.", Toast.LENGTH_SHORT).show();
            return;
        }

        if(TextUtils.isEmpty(password))
        {
            //pass empty
            Toast.makeText(this, "Please enter password.", Toast.LENGTH_SHORT).show();
            return;
        }

        progressDialog.setMessage("Registering User....");
        progressDialog.show();
        mAuth.createUserWithEmailAndPassword(email,password).addOnCompleteListener(this, new OnCompleteListener<AuthResult>() {
            @Override
            public void onComplete(@NonNull Task<AuthResult> task) {
                if(task.isSuccessful())
                {
                    Toast.makeText(MainActivity.this, "Success!", Toast.LENGTH_SHORT).show();
                }
                else
                {
                    Toast.makeText(MainActivity.this, "FAIL T_T", Toast.LENGTH_SHORT).show();
                }
            }
        });

    }

    private void userLogin()
    {
        String email = editTextemail.getText().toString().trim();
        String password = editTextpass.getText().toString().trim();

        if(TextUtils.isEmpty(email))
        {
            //email empty
            Toast.makeText(this, "Please enter email.", Toast.LENGTH_SHORT).show();
            return;
        }

        if(TextUtils.isEmpty(password))
        {
            //pass empty
            Toast.makeText(this, "Please enter password.", Toast.LENGTH_SHORT).show();
            return;
        }

        progressDialog.setMessage("Logging in....");
        progressDialog.show();

        mAuth.signInWithEmailAndPassword(email,password).addOnCompleteListener(this, new OnCompleteListener<AuthResult>() {
            @Override
            public void onComplete(@NonNull Task<AuthResult> task) {
                progressDialog.dismiss();

                if(task.isSuccessful())
                {
                    finish();
                    startActivity(new Intent(getApplicationContext(),gameActivity.class));
                }
                else
                {
                    Toast.makeText(MainActivity.this, "Please check information", Toast.LENGTH_SHORT).show();
                    return;
                }


            }
        });





    }
/*
    @Override
    public void onStart() {
        super.onStart();
        mAuth.addAuthStateListener(mAuthListener);
    }

    @Override
    public void onStop() {
        super.onStop();
        if (mAuthListener != null) {
            mAuth.removeAuthStateListener(mAuthListener);
        }
    }
*/



}
