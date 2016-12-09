package com.runmit.sweedee.player.subtitle;

import java.io.File;

import com.runmit.sweedee.util.XL_Log;

import android.content.Context;
import android.database.sqlite.SQLiteCantOpenDatabaseException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.os.Environment;
import android.util.Log;

public abstract class SDSQLiteOpenHelper {
    private static final String TAG = SDSQLiteOpenHelper.class.getSimpleName();

    @SuppressWarnings("unused")
    private final Context mContext;
    public final String mFullName;
    private final CursorFactory mFactory;
    private final int mNewVersion;


    private boolean mIsInitializing = false;

    /**
     * Create a helper object to create, open, and/or manage a database. The
     * database is not actually created or opened until one of
     * {@link #getWritableDatabase} or {@link #getReadableDatabase} is called.
     *
     * @param context to use to open or create the database
     * @param name    of the database file, or null for an in-memory database
     * @param factory to use for creating cursor objects, or null for the default
     * @param version number of the database (starting at 1); if the database is
     *                older, {@link #onUpgrade} will be used to upgrade the database
     */
    public SDSQLiteOpenHelper(Context context, String fullnamepath,
                              CursorFactory factory, int version) {
        XL_Log log = new XL_Log(SDSQLiteOpenHelper.class);
        mContext = context;
        mFullName = fullnamepath;
        mFactory = factory;
        mNewVersion = version;

        SQLiteDatabase db = null;
        if (mFullName == null) {
            db = SQLiteDatabase.create(null);
        } else {
            try {
                String path = mFullName;
                db = SQLiteDatabase.openOrCreateDatabase(path, mFactory);
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (db == null)
                    return;
            }
        }

        int current_version = db.getVersion();
        if (current_version != mNewVersion) {
            db.beginTransaction();
            try {
                if (current_version == 0) {
                    onCreate(db);
                }
                db.setTransactionSuccessful();
            } finally {
                db.endTransaction();
            }
        }
        db.close();
    }

    /**
     * Create and/or open a database that will be used for reading and writing.
     * Once opened successfully, the database is cached, so you can call this
     * method every time you need to write to the database. Make sure to call
     * {@link #close} when you no longer need it.
     * <p/>
     * <p>
     * Errors such as bad permissions or a full disk may cause this operation to
     * fail, but future attempts may succeed if the problem is fixed.
     * </p>
     *
     * @return a read/write database object valid until {@link #close} is called
     * @throws SQLiteException if the database cannot be opened for writing
     */
    public synchronized SQLiteDatabase getWritableDatabase() {


        if (mIsInitializing) {
            throw new IllegalStateException(
                    "getWritableDatabase called recursively");
        }

        // If we have a read-only database open, someone could be using it

        // (though they shouldn't), which would cause a lock to be held on

        // the file, and our attempts to open the database read-write would

        // fail waiting for the file lock. To prevent that, we acquire the

        // lock on the read-only database, which shuts out other users.

        boolean success = false;
        SQLiteDatabase db = null;
        try {
            mIsInitializing = true;
            db = SQLiteDatabase.openOrCreateDatabase(mFullName, mFactory);
            onOpen(db);
            success = true;
            return db;
        } finally {
            mIsInitializing = false;
            if (success) {

            } else {
                if (db != null)
                    db.close();
            }
        }
    }

    public int getVersion() {
        SQLiteDatabase db = null;
        final String path = mFullName;
        int version = 0;
        try {
            db = SQLiteDatabase.openOrCreateDatabase(path, mFactory);
            version = db.getVersion();
            db.close();
        } catch (SQLiteException e) {
            e.printStackTrace();
        }
        return version;
    }

    public void setVersion(int version) {
        try {
            SQLiteDatabase db = null;
            final String path = mFullName;
            db = SQLiteDatabase.openOrCreateDatabase(path, mFactory);
            db.setVersion(version);
            db.close();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    /**
     * Create and/or open a database. This will be the same object returned by
     * {@link #getWritableDatabase} unless some problem, such as a full disk,
     * requires the database to be opened read-only. In that case, a read-only
     * database object will be returned. If the problem is fixed, a future call
     * to {@link #getWritableDatabase} may succeed, in which case the read-only
     * database object will be closed and the read/write object will be returned
     * in the future.
     *
     * @return a database object valid until {@link #getWritableDatabase} or
     * {@link #close} is called.
     * @throws SQLiteException if the database cannot be opened
     */
    public synchronized SQLiteDatabase getReadableDatabase() {
        SQLiteDatabase db = SQLiteDatabase.openDatabase(mFullName, mFactory,
                SQLiteDatabase.OPEN_READONLY);
        onOpen(db);
        return db;
    }

//	/**
//	 * Close any open database object.
//	 */
//	public synchronized void close() {
//		if (mIsInitializing)
//			throw new IllegalStateException("Closed during initialization");
//
////		if (mDatabase != null && mDatabase.isOpen()) {
////			mDatabase.close();
////			mDatabase = null;
////		}
//	}

    /**
     * Called when the database is created for the first time. This is where the
     * creation of tables and the initial population of the tables should
     * happen.
     *
     * @param db The database.
     */
    public abstract void onCreate(SQLiteDatabase db);

    /**
     * Called when the database needs to be upgraded. The implementation should
     * use this method to drop tables, add tables, or do anything else it needs
     * to upgrade to the new schema version.
     * <p/>
     * <p/>
     * The SQLite ALTER TABLE documentation can be found <a
     * href="http://sqlite.org/lang_altertable.html">here</a>. If you add new
     * columns you can use ALTER TABLE to insert them into a live table. If you
     * rename or remove columns you can use ALTER TABLE to rename the old table,
     * then create the new table and then populate the new table with the
     * contents of the old table.
     *
     * @param db         The database.
     * @param oldVersion The old database version.
     * @param newVersion The new database version.
     */
    public abstract void onUpgrade(SQLiteDatabase db, int oldVersion,
                                   int newVersion);

    /**
     * Called when the database has been opened. Override method should check
     * {@link SQLiteDatabase#isReadOnly} before updating the database.
     *
     * @param db The database.
     */
    public void onOpen(SQLiteDatabase db) {
    }
}
