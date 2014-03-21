/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.0
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opentransactions.otapi;

public class ServerInfo extends Displayable {
  private long swigCPtr;

  protected ServerInfo(long cPtr, boolean cMemoryOwn) {
    super(otapiJNI.ServerInfo_SWIGUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(ServerInfo obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        otapiJNI.delete_ServerInfo(swigCPtr);
      }
      swigCPtr = 0;
    }
    super.delete();
  }
/*@SWIG:swig\otapi\OTAPI.i,158,OT_CAN_BE_CONTAINED_BY@*/
	// Ensure that the GC doesn't collect any OT_CONTAINER instance set from Java
	private ContactNym containerRefContactNym;
	// ----------------	
	protected void addReference(ContactNym theContainer) {  // This is Java code
		containerRefContactNym = theContainer;
	}
	// ----------------
/*@SWIG@*/
	// ------------------------
  public void setGui_label(String value) {
    otapiJNI.ServerInfo_gui_label_set(swigCPtr, this, value);
  }

  public String getGui_label() {
    return otapiJNI.ServerInfo_gui_label_get(swigCPtr, this);
  }

  public void setServer_id(String value) {
    otapiJNI.ServerInfo_server_id_set(swigCPtr, this, value);
  }

  public String getServer_id() {
    return otapiJNI.ServerInfo_server_id_get(swigCPtr, this);
  }

  public void setServer_type(String value) {
    otapiJNI.ServerInfo_server_type_set(swigCPtr, this, value);
  }

  public String getServer_type() {
    return otapiJNI.ServerInfo_server_type_get(swigCPtr, this);
  }

  public static ServerInfo ot_dynamic_cast(Storable pObject) {
    long cPtr = otapiJNI.ServerInfo_ot_dynamic_cast(Storable.getCPtr(pObject), pObject);
    return (cPtr == 0) ? null : new ServerInfo(cPtr, false);
  }

}
