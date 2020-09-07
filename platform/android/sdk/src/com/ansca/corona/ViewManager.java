//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona;

import com.ansca.corona.permissions.PermissionsServices;
import com.ansca.corona.permissions.PermissionsSettings;
import com.ansca.corona.permissions.PermissionState;

import java.io.File;
import java.util.HashMap;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.InputMethodManager;
import android.webkit.CookieManager;
import android.widget.AbsoluteLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;

public class ViewManager {
	private java.util.ArrayList<android.view.View> myDisplayObjects;
	private Context myContext;
	private ViewGroup myContentView;
	private android.widget.FrameLayout myOverlayView;
	private android.widget.AbsoluteLayout myAbsoluteViewLayout;
	private CoronaRuntime myCoronaRuntime;
	private Handler myHandler;
	
	/**
	 * HashMap derived class which uses type String for the keys and type Object for the values.
	 * Instances of this class are tagged to views for storing various information.
	 */
	private class StringObjectHashMap extends java.util.HashMap<String, Object> { }
	
	
	public ViewManager(Context context, CoronaRuntime runtime)
	{
		myContext = context;
		myCoronaRuntime = runtime;
		myDisplayObjects =  new java.util.ArrayList<android.view.View>();
		myContentView = null;
		myOverlayView = null;
		myAbsoluteViewLayout = null;

		// The main looper lives on the main thread of the application.  
		// This will bind the handler to post all runnables to the main ui thread.
		myHandler = new Handler(Looper.getMainLooper());
	}
	
	/**
	 * Gets the root view belonging to the CoronaActivity which was assigned to it via Activity.setContentView() method.
	 * This view contains all other views such as the OpenGL view, overlay view, and the absolute layout view.
	 * @return Returns the root view on the activity the contains all other views.
	 */
	public ViewGroup getContentView()
	{
		return myContentView;
	}
	
	/**
	 * Gets the view that is overlaid on top of the OpenGL view.
	 * UI objects such as text fields, video views, web views, ads, etc. should be added to this view.
	 * @return Returns a FrameLayout view. Returns null if its CoronaActivity is not available.
	 */
	public android.widget.FrameLayout getOverlayView()
	{
		return myOverlayView;
	}
	
	/**
	 * Gets an absolute layout view group contained with the overlay view.
	 * This view allows you to set pixel position of UI objects on top of the OpenGL view.
	 * @return Returns an AbsoluteLayout object. Returns null if its CoronaActivity is not available.
	 */
	public android.widget.AbsoluteLayout getAbsoluteViewLayout()
	{
		return myAbsoluteViewLayout;
	}
	
	/**
	 * Fetches a Corona display object by its unique ID.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns the specified display object. Returns null if not found.
	 */
	public android.view.View getDisplayObjectById(int id) {
		synchronized (myDisplayObjects) {
			for (android.view.View view : myDisplayObjects) {
				if ((view != null) && (view.getId() == id)) {
					return view;
				}
			}
		}
		return null;
	}
	
	/**
	 * Fetches a Corona display object by its unique ID.
	 * @param type The type of display object to look for, such as "CoronaEditText". Must derive from the "View" class.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns the specified display object. Returns null if not found.
	 */
	public <T extends android.view.View> T getDisplayObjectById(Class<T> type, int id) {
		synchronized (myDisplayObjects) {
			for (android.view.View view : myDisplayObjects) {
				if (type.isInstance(view) && (view.getId() == id)) {
					return (T)view;
				}
			}
		}
		return null;
	}
	
	/**
	 * Fetches a CoronaVideoView object by its unique ID.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns the specified display object. Returns null if not found.
	 */
	private CoronaVideoView getCoronaVideoViewById(int id) {
		CoronaVideoView.CenteredLayout videoLayout = getDisplayObjectById(CoronaVideoView.CenteredLayout.class, id);
		if (videoLayout != null) {
			return videoLayout.getVideoView();
		}
		return getDisplayObjectById(CoronaVideoView.class, id);
	}

	/**
	 * Determines if a display object having the given ID exists.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns true if a display object having the given ID exists. Returns false if not.
	 */
	public boolean hasDisplayObjectWithId(int id) {
		return (getDisplayObjectById(id) != null);
	}
	
	/**
	 * Determines if a display object having the given ID exists.
	 * @param type The type of display object to look for, such as "CoronaEditText". Must derive from the "View" class.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns true if a display object having the given ID exists. Returns false if not.
	 */
	public <T extends android.view.View> boolean hasDisplayObjectWithId(Class<T> type, int id) {
		return (getDisplayObjectById(type, id) != null);
	}
	
	/**
	 * Posts the given Runnable object to the main UI thread's message queue in a thread safe manner.
	 * @param runnable The Runnable object to be posted and executed on the main UI thread.
	 */
	private void postOnUiThread(Runnable runnable) {
		// Validate.
		if (runnable == null) {
			return;
		}
		
		if (myHandler != null &&
			myHandler.getLooper() != null &&
			myHandler.getLooper().getThread() != null &&
			!myHandler.getLooper().getThread().isAlive()) {

			return;
		}
			
		myHandler.post(runnable);
	}
	
	public void suspend() {
		// Traverse all views and stop any operations that they are currently performing.
		synchronized (myDisplayObjects) {
			for (android.view.View view : myDisplayObjects) {
				if (view instanceof CoronaVideoView) {
					// Suspend video playback.
					((CoronaVideoView)view).suspend();
				}
				else if (view instanceof CoronaVideoView.CenteredLayout) {
					// Suspend video playback.
					((CoronaVideoView.CenteredLayout)view).getVideoView().suspend();
				}
			}
		}
	}

	public void resume() {
		// Traverse all views and restart them.
		synchronized (myDisplayObjects) {
			for (android.view.View view : myDisplayObjects) {
				if (view instanceof CoronaVideoView) {
					// Resume video's last playback state.
					((CoronaVideoView)view).resume();
				}
				else if (view instanceof CoronaVideoView.CenteredLayout) {
					// Resume video's last playback state.
					((CoronaVideoView.CenteredLayout)view).getVideoView().resume();
				}
			}
		}
	}

	public void addTextView(
		final int id, final int left, final int top, final int width, final int height, final boolean isSingleLine )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				// Do not continue if the view is no longer available.
				if (myAbsoluteViewLayout == null) {
					return;
				}
				
				// Create the text field with the given settings.
				CoronaEditText editText = new CoronaEditText(myContext, myCoronaRuntime);
				LayoutParams params = new AbsoluteLayout.LayoutParams(
						width + editText.getBorderPaddingLeft() + editText.getBorderPaddingRight(),
						height + editText.getBorderPaddingTop() + editText.getBorderPaddingBottom(),
						left - editText.getBorderPaddingLeft(),
						top - editText.getBorderPaddingTop());
				myAbsoluteViewLayout.addView( editText, params );
				editText.setId(id);
				editText.setTag(new StringObjectHashMap());
				editText.bringToFront();
				editText.setTextColor( Color.BLACK );
				editText.setSingleLine(isSingleLine);
				editText.setImeOptions(android.view.inputmethod.EditorInfo.IME_ACTION_DONE);

				// Set the vertical alignment to "center" for single line fields and "top" for multiline fields.
				int gravity = editText.getGravity() & android.view.Gravity.HORIZONTAL_GRAVITY_MASK;
				gravity |= (isSingleLine ? android.view.Gravity.CENTER_VERTICAL : android.view.Gravity.TOP);
				editText.setGravity(gravity);
				
				// Do not allow the text field to change focus to another field to match iOS behavior.
				editText.setNextFocusDownId(editText.getId());
				editText.setNextFocusUpId(editText.getId());
				editText.setNextFocusLeftId(editText.getId());
				editText.setNextFocusRightId(editText.getId());
				
				// Add the text field to the display object collection to be made accessible to the native side of Corona.
				synchronized (myDisplayObjects) {
					myDisplayObjects.add(editText);
				}
			}
		} );
	}
	
	public void setTextViewInputType( final int id, final String inputType )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;

				view.setTextViewInputType(inputType);
			}
		} );
	}
	
	public String getTextViewInputType( int id )
	{
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if ( view == null )
			return "error";
		
		return view.getTextViewInputType();
	}

	public void setTextViewSingleLine(final int id, final boolean isSingleLine)
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if (view != null) {
					view.setSingleLine(isSingleLine);
				}
			}
		} );
	}
	
	public boolean isTextViewSingleLine(int id) {
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if (view == null) {
			return true;
		}
		return ((view.getInputType() & android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE) == 0);
	}
	
	public void setTextViewEditable(final int id, final boolean isEditable) {
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if (view != null) {
					view.setReadOnly(!isEditable);
					view.setFocusable(isEditable);
					view.setFocusableInTouchMode(isEditable);
				}
			}
		} );
	}
	
	public boolean isTextViewEditable(int id) {
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if (view == null) {
			return true;
		}
		return view.isEnabled();
	}
	
	public void setTextViewPassword( final int id, final boolean isPassword )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;

				view.setTextViewPassword(isPassword);
			}
		} );
	}

	public boolean getTextViewPassword( int id )
	{
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if ( view == null )
			return false;
		
		return view.getTextViewPassword();
	}

	public void setTextViewAlign( final int id, final String align )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;

				view.setTextViewAlign(align);
			}
		} );
	}
	
	public String getTextViewAlign( int id )
	{
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if ( view == null )
			return "";

		return view.getTextViewAlign();
	}
	
	public void setTextReturnKey( final int id, final String imeType)
	{
		postOnUiThread( new Runnable() {
			public void run() {
				int ime = EditorInfo.IME_ACTION_DONE;
				if (imeType.equals("go")) {
					ime = EditorInfo.IME_ACTION_GO;
				} else if (imeType.equals("next")) {
					ime = EditorInfo.IME_ACTION_NEXT;
				} else if (imeType.equals("none")) {
					ime = EditorInfo.IME_ACTION_NONE;
				} else if (imeType.equals("search")) {
					ime = EditorInfo.IME_ACTION_SEARCH;
				} else if (imeType.equals("send")) {
					ime = EditorInfo.IME_ACTION_SEND;
				}

				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;
				view.setImeOptions(ime);
			}
		} );
	}

	public void setTextPlaceholder( final int id, final String placeholder )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null ) {
					return;
				}
				view.setHint(placeholder);
			}
		} );
	}

	public void setTextSelection( final int id, final int startPositionFinal, final int endPositionFinal )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;
				int maxLength = view.getText().length();
				int startPosition = startPositionFinal;
				int endPosition = endPositionFinal;
				if (startPosition > maxLength) {
					startPosition = maxLength;
					endPosition = maxLength;
				}

				if (endPosition > maxLength) {
					endPosition = maxLength;
				}

				if (startPosition < 0) {
					startPosition = 0;
				}

				if (endPosition < 0) {
					endPosition = 0;
				}

				if (startPosition > endPosition) {
					startPosition = endPosition;
				}

				view.setSelection(startPosition, endPosition);
			}
		} );
	}

	public void setTextViewColor( final int id, final int color )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;
				view.setTextViewColor( color );
			}
		} );
	}
	
	public int getTextViewColor( int id )
	{
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if ( view == null )
			return 0;
		return view.getTextViewColor();
	}
	
	public void setTextViewSize( final int id, final float size )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;
				view.setTextViewSize( size );
			}
		} );
	}
	
	public float getTextViewSize( int id )
	{
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if ( view == null )
			return 0.0f;
		return view.getTextViewSize();
	}
	
	public void setTextViewFocus( final int id, final boolean focus )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = null;
				if (id != 0) {
					view = getDisplayObjectById(CoronaEditText.class, id);
				}
				if ((view != null) && focus) {
					// This puts a focus ring around the text box.
					view.requestFocus();

					// We need the following to bring up the keyboard.
					InputMethodManager inputManager = (InputMethodManager) myContext.getSystemService(Context.INPUT_METHOD_SERVICE); 
					
					// Not sure if we should call SHOW_FORCED or SHOW_IMPLICIT.
					// I think the latter will only appear if there is no hardware keyboard.
					// According to the docs, only SHOW_IMPLICIT is supported, but SHOW_FORCED works in my testing.
					inputManager.showSoftInput(view, InputMethodManager.SHOW_FORCED); 
				}
				else {
					// Set the focus to the main window. This removes the focus ring from the last field.
					myContentView.requestFocus();
					
					// Hide the virtual keyboard, if displayed.
					InputMethodManager imm = (InputMethodManager)myContext.getSystemService(Context.INPUT_METHOD_SERVICE);
					imm.hideSoftInputFromWindow(myContentView.getApplicationWindowToken(), 0);
				}
			}
		} );
	}
	
	public void setTextViewText( final int id, final String text ) {
		postOnUiThread( new Runnable() {
			public void run() {
				// Fetch the requested EditText.
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if (view == null) {
					return;
				}
				
				// Get the current cursor position(s).
				int selectionStartIndex = view.getSelectionStart();
				int selectionEndIndex = view.getSelectionEnd();

				// Update the field's text.
				view.setText(text);

				// Restore the cursor position(s).
				int textLength = view.getText().length();
				if (selectionStartIndex > textLength) {
					selectionStartIndex = textLength;
				}
				if (selectionEndIndex > textLength) {
					selectionEndIndex = textLength;
				}
				selectionStartIndex = Math.max(0, selectionStartIndex);
				selectionEndIndex = Math.max(0, selectionEndIndex);
				view.setSelection(selectionStartIndex, selectionEndIndex);
			}
		} );
	}
	
	public void setTextViewFont(
		final int id, final String fontName, final float fontSize, final boolean isBold)
	{
		postOnUiThread( new Runnable() {
			public void run() {
				CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
				if ( view == null )
					return;
				
				view.setTextViewFont(fontName, fontSize, isBold);
			}
		} );
	}
	
	public String getTextViewText( int id )
	{
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if ( view == null ) {
			return "";
		}
		
		return view.getTextString();
	}
	
	public String getTextViewPlaceholder( int id )
	{
		CoronaEditText view = getDisplayObjectById(CoronaEditText.class, id);
		if ( view == null ) {
			return null;
		}
		CharSequence text = view.getHint();
		return (text != null) ? text.toString() : null;
	}

	public void setGLView(View glView)
	{
		// Set up the root content view that will contain all other views.
		// A FrameLayout is best because its child views be rendered on top of each other in the order that they were added.
		// Make this view focusable so that we can clear the focus from native fields by setting the focus to the container.
		myContentView = new android.widget.FrameLayout(myContext);
		myContentView.setFocusable(true);
		myContentView.setFocusableInTouchMode(true);
		
		// Add the given OpenGL view to the root content view first.
		// This way it is at the back of the z-order so that all other views are overlaid on top.
		myContentView.addView(glView);
		
		// Add an invisible overlay view to the root content view.
		// This is a view container for all UI objects such as text fields, web views, video views, ads, etc.
		// Add an AbsoluteLayout to this overlay view, which can be used to set pixel positions of native fields.
		myOverlayView = new android.widget.FrameLayout(myContext);
		myAbsoluteViewLayout = new android.widget.AbsoluteLayout(myContext);
		myOverlayView.addView(myAbsoluteViewLayout);
		myContentView.addView(myOverlayView);
	}

	public void destroy() {
		destroyAllDisplayObjects();
	}
	
	public void destroyAllDisplayObjects() {
		View view;

		// Destroy all display objects in the collection.
		do {
			// Fetch the next display object.
			synchronized (myDisplayObjects) {
				if (myDisplayObjects.isEmpty()) {
					view = null;
				}
				else {
					view = myDisplayObjects.get(myDisplayObjects.size() - 1);
				}
			}

			// Destroy the display object.
			if (view != null) {
				destroyDisplayObject(view.getId());
			}
		} while (view != null);
	}
	
	public void destroyDisplayObject( final int id ) {
		// Fetch the display object.
		final View view = getDisplayObjectById(id);
		if (view == null) {
			return;
		}
		
		// Remove the display object from the collection before destroying it on the UI thread.
		synchronized (myDisplayObjects) {
			myDisplayObjects.remove(view);
		}
		
		// Remove the display object via the UI thread.
		postOnUiThread(new Runnable() {
			@Override
			public void run() {
				// Remove the view from the layout.
				android.view.ViewParent parentView = view.getParent();
				if ((parentView != null) && (parentView instanceof android.view.ViewGroup)) {
					((android.view.ViewGroup)parentView).removeView(view);
				}

				// Set the view's ID to an invalid value.
				// This causes any upcoming events that may get raised by the destroyed view to be ignored.
				// Some views send an event upon destruction, so changing this ID must come after destroying them.
				view.setId(0);
			}
		});
	}
	
	public void setDisplayObjectVisible( final int id, final boolean visible ) {
		postOnUiThread( new Runnable() {
			public void run() {
				View view = getDisplayObjectById(id);
				if (view != null) {
					view.setVisibility(visible ? View.VISIBLE : View.GONE);
					if (visible) {
						setDisplayObjectAlpha(id, getDisplayObjectAlpha(id));
					}
					else {
						view.setAnimation(null);
					}
				}
			}
		} );
	}
	
	public void displayObjectUpdateScreenBounds(
		final int id, final int left, final int top, final int width, final int height )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				View view = getDisplayObjectById(id);
				if ( view != null ) {
					LayoutParams params = null;
					if (view instanceof CoronaEditText) {
						CoronaEditText editText = (CoronaEditText)view;
						params = new AbsoluteLayout.LayoutParams(
								width + editText.getBorderPaddingLeft() + editText.getBorderPaddingRight(),
								height + editText.getBorderPaddingTop() + editText.getBorderPaddingBottom(),
								left - editText.getBorderPaddingLeft(),
								top - editText.getBorderPaddingTop());
					}
					else {
						params = new AbsoluteLayout.LayoutParams( width, height, left, top );
					}
					view.setLayoutParams(params);
				}
			}
		} );
	}
	
	public boolean getDisplayObjectVisible( int id ) {
		boolean result = false;
		
		View view = getDisplayObjectById(id);
		if ( view != null ) {
			result = (view.getVisibility() == View.VISIBLE);
		}
		return result;
	}
	
	public float getDisplayObjectAlpha(int id) {
		float alpha = 1.0f;
		
		View view = getDisplayObjectById(id);
		if (view != null) {
			Object tag = view.getTag();
			if (tag instanceof StringObjectHashMap) {
				Object value = ((StringObjectHashMap)tag).get("alpha");
				if (value instanceof Float) {
					alpha = ((Float)value).floatValue();
				}
			}
		}
		return alpha;
	}
	
	public void setDisplayObjectAlpha(final int id, final float alpha) {
		postOnUiThread(new Runnable() {
			public void run() {
				View view = getDisplayObjectById(id);
				if (view != null) {
					// Do not allow the alpha color to exceed its bounds.
					float newAlpha = alpha;
					if (newAlpha < 0) {
						newAlpha = 0;
					}
					else if (newAlpha > 1.0f) {
						newAlpha = 1.0f;
					}
					
					// Have the view store the new alpha value to be retrieved by the getDisplayObjectAlpha() method.
					Object tag = view.getTag();
					if (tag instanceof StringObjectHashMap) {
						((StringObjectHashMap)tag).put("alpha", Float.valueOf(newAlpha));
					}
					
					// Alpha blend the view via an animation object.
					if ((newAlpha < 0.9999f) && (view.getVisibility() == View.VISIBLE)) {
						android.view.animation.AlphaAnimation animation;
						animation = new android.view.animation.AlphaAnimation(1.0f, newAlpha);
						animation.setDuration(0);
						animation.setFillAfter(true);
						view.startAnimation(animation);
					}
					else {
						view.setAnimation(null);
					}
				}
			}
		});
	}
	
	public void setDisplayObjectBackground( final int id, final boolean isVisible ) {
		postOnUiThread( new Runnable() {
			public void run() {
				// Fetch the view.
				View view = getDisplayObjectById(id);
				if (view == null) {
					return;
				}
				
				// Fetch the view's background drawable object.
				// Note that it will be null if the background has been hidden.
				android.graphics.drawable.Drawable background = view.getBackground();
				
				// Do not continue if the background visibility state isn't going to changing.
				if ((isVisible && (background != null)) ||
				    (!isVisible && (background == null))) {
					return;
				}
				
				// If the background was not found, then attempt to fetch it from the HashMap tagged to the view.
				StringObjectHashMap hashMap = null;
				Object tag = view.getTag();
				if (tag instanceof StringObjectHashMap) {
					hashMap = (StringObjectHashMap)tag;
				}
				if ((background == null) && (hashMap != null)) {
					Object value = hashMap.get("backgroundDrawable");
					if (value instanceof android.graphics.drawable.Drawable) {
						background = (android.graphics.drawable.Drawable)value;
					}
				}
				
				// Hide the background by settings the background drawable to null.
				// Show the background by assigning the view the last displayed background.
				// If the last displayed background is unknown, then generate a new background drawable.
				if (isVisible && (background == null)) {
					view.setBackgroundColor(android.graphics.Color.WHITE);
					background = view.getBackground();
				}
				else {
					view.setBackgroundDrawable(isVisible ? background : null);
				}
				
				// If we are hiding the background, then store it to the HashMap to be restored later.
				if ((isVisible == false) && (hashMap != null)) {
					hashMap.put("backgroundDrawable", background);
				}
				
				// Redraw the view.
				view.invalidate();
			}
		} );
	}
	
	public boolean getDisplayObjectBackground(final int id) {
		boolean hasBackground = false;
		
		// Fetch the view.
		View view = getDisplayObjectById(id);
		if (view == null) {
			return false;
		}
		
		// Determine if there is a background by looking at its drawable object.
		android.graphics.drawable.Drawable background = view.getBackground();
		if (background instanceof android.graphics.drawable.ColorDrawable) {
			hasBackground = (((android.graphics.drawable.ColorDrawable)background).getAlpha() > 0);
		}
		else if (background instanceof android.graphics.drawable.ShapeDrawable) {
			android.graphics.Paint paint = ((android.graphics.drawable.ShapeDrawable)background).getPaint();
			if (paint != null) {
				hasBackground = (paint.getColor() != android.graphics.Color.TRANSPARENT);
			}
		}
		else if (background != null) {
			hasBackground = true;
		}
		return hasBackground;
	}
	
	private void setHardwareAccelerationEnabled(View view, boolean enabled) {
		// Validate.
		if (view == null) {
			return;
		}
		
		// Enable/disable hardware acceleration via reflection.
		try {
			java.lang.reflect.Method setLayerTypeMethod = android.view.View.class.getMethod(
					"setLayerType", new Class[] {Integer.TYPE, android.graphics.Paint.class});
			int layerType = enabled ? 2 : 1;
			setLayerTypeMethod.invoke(view, new Object[] {layerType, null});
		}
		catch (Exception ex) { }
	}
	
	public void addVideoView(final int id, final int left, final int top, final int width, final int height)
	{
		postOnUiThread(new Runnable() {
			public void run() {
				// Validate.
				if ((myContext == null) || (myAbsoluteViewLayout == null)) {
					return;
				}

				// Create a video view centered within a FrameLayout container.
				CoronaVideoView.CenteredLayout view = new CoronaVideoView.CenteredLayout(myContext, myCoronaRuntime);
				view.setId(id);
				view.setTag(new StringObjectHashMap());
				LayoutParams layoutParams = new AbsoluteLayout.LayoutParams(width, height, left, top);
				myAbsoluteViewLayout.addView(view, layoutParams);
				view.bringToFront();
				view.getVideoView().setZOrderOnTop(true);

				// Add the view to the display object collection to be made accessible to the native side of Corona.
				synchronized (myDisplayObjects) {
					myDisplayObjects.add(view);
				}
			}
		});
	}

	public void videoViewLoad(final int id, final String path) {
		postOnUiThread(new Runnable() {
			public void run() {
				CoronaVideoView view = getCoronaVideoViewById(id);
				MediaManager mediaManager = myCoronaRuntime.getController().getMediaManager();
				if (view != null && mediaManager != null) {
					view.setVideoURIUsingCoronaProxy(mediaManager.createVideoURLFromString(path, myContext));
				}
			}
		});
	}
	
	public void videoViewPlay(final int id) {
		postOnUiThread(new Runnable() {
			public void run() {
				CoronaVideoView view = getCoronaVideoViewById(id);
				if (view != null) {
					view.start();
				}
			}
		});
	}

	public void videoViewPause(final int id) {
		postOnUiThread(new Runnable() {
			public void run() {
				CoronaVideoView view = getCoronaVideoViewById(id);
				if (view != null) {
					view.pause();
				}
			}
		});
	}

	public void videoViewSeek(final int id, final int time) {
		postOnUiThread(new Runnable() {
			public void run() {
				CoronaVideoView view = getCoronaVideoViewById(id);
				if (view != null) {
					// The time we get from lua is in seconds but the seekTo function takes milliseconds
					view.seekTo(time*1000);
				}
			}
		});
	}

	public int videoViewGetCurrentTime(final int id) {
		CoronaVideoView view = getCoronaVideoViewById(id);
		if (view != null) {
			// The time we get from the view is in milliseconds but we want to return it in seconds
			return view.getCurrentPosition()/1000;
		}
		return 0;
	}

	public int videoViewGetTotalTime(final int id) {
		CoronaVideoView view = getCoronaVideoViewById(id);
		if (view != null) {
			return view.getDuration()/1000;
		}
		return 0;
	}

	public boolean videoViewGetIsMuted(final int id) {
		CoronaVideoView view = getCoronaVideoViewById(id);
		if (view != null) {
			return view.isMuted();
		}
		return false;
	}

	public void videoViewMute(final int id, final boolean mute) {
		postOnUiThread(new Runnable() {
			public void run() {
				CoronaVideoView view = getCoronaVideoViewById(id);
				if (view != null) {
					view.mute(mute);
				}
			}
		});
	}

	public boolean videoViewGetIsTouchTogglesPlay(final int id) {
		CoronaVideoView view = getCoronaVideoViewById(id);
		if (view != null) {
			return view.isTouchTogglesPlay();
		}
		return false;
	}

	public void videoViewTouchTogglesPlay(final int id, final boolean toggle) {
		postOnUiThread(new Runnable() {
			public void run() {
				CoronaVideoView view = getCoronaVideoViewById(id);
				if (view != null) {
					view.touchTogglesPlay(toggle);
				}
			}
		});
	}
	
	public boolean videoViewGetIsPlaying(final int id) {
		CoronaVideoView view = getCoronaVideoViewById(id);
		if (view != null) {
			return view.isPlaying();
		}
		return false;
	}
	
}
